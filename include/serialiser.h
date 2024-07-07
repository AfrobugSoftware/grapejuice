#pragma once
#include <boost/asio/buffer.hpp>
#include <boost/fusion/include/adapter.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/uuid/uuid.hpp>

#include "currency.h"

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <bit>
#include <bitset>

namespace grape
{
	template<typename T>
	concept Integers = std::is_integral_v<T>  || std::is_floating_point_v<T>;

	template<typename T>
	concept Pods = std::is_pod_v<T> && !Integers<T>;

	template<typename T>
	concept Enums = std::is_enum_v <T>;

	template<typename T>
	concept FusionStruct = boost::mpl::is_sequence<T>::value;

	template<Integers T>
	auto bswap(const T& value) -> T {
		auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
		std::ranges::reverse(value_representation);
		return std::bit_cast<T>(value_representation);
	}

	template<typename T, size_t N = CHAR_BIT * sizeof(T)>
	struct optional_field_set {
		using value_type = T;
		using bits_type = std::bitset<N>;
	};

	template<typename T, size_t N>
	struct optional_field : std::optional<T> {
		constexpr static const size_t bit = N;
	};

	using opt_fields = optional_field_set<std::uint16_t>;

	namespace serial {
		class reader {
		public:
			mutable boost::asio::const_buffer buf_;
			mutable std::optional<opt_fields::bits_type> opt_;

			reader(boost::asio::const_buffer buf) : buf_{ std::move(buf) }, opt_{std::nullopt} {}
			
			template<Integers T>
			void operator()(T& i) const {
				i = bswap(*boost::asio::buffer_cast<const T*>(buf_));
				buf_ += sizeof(T);
			}

			void operator()(boost::uuids::uuid& uuid) const {
				memcpy(uuid.data, buf_.data(), uuid.static_size());
				buf_ += uuid.static_size();
			}

			void operator()(std::string& str) const {
				std::uint32_t len = 0;
				(*this)(len);
				str = std::string(reinterpret_cast<const char*>(buf_.data()), len);
				buf_ += len;
			}	

			void operator()(pof::base::currency& cur) const {
				memcpy(cur.data().data(), buf_.data(), cur.data().size());
				buf_ += cur.data().size();
			}
			template<Pods P>
			void operator()(P& p) const {
				memcpy(&p, buf_.data(), sizeof(P));
				buf_ += sizeof(P);
			}

			template<size_t N>
			void operator()(std::array<char, N>& fixed) const {
				memcpy(fixed.data(), buf_.data(), N);
				buf_ += N;
			}

			template<FusionStruct T>
			void operator()(T& val) const {
				boost::fusion::for_each(val, *this);
			}
			template<typename T, size_t N>
			void operator()(opt_fields) const {
				opt_fields::value_type val;
				(*this)(val);
				opt_ = opt_fields::bits_type(val);
			}

			template<class T, size_t N>
			void operator()(optional_field<T, N>& val) const {
				if (!opt_.has_value()) throw std::logic_error("optional field comes before optional set");;
				if ((*opt_)[N]) {
					T v{};
					(*this)(v);
					val = v;
				}
			}

			template<typename T> 
				requires Integers<T> || FusionStruct<T> || Enums<T> || Pods<T>
			void operator()(std::vector<T>& vec) const
			{
				std::uint32_t len = 0;
				(*this)(len);
				vec.reserve(len);
				for (; len; --len) {
					T v{};
					(*this)(v);
					vec.emplace_back(std::move(v));
				}
			}
		};

		class writer {
		public:
			mutable boost::asio::mutable_buffer buf_;
			mutable opt_fields::bits_type opt_;
			mutable opt_fields::value_type* optv_;

			writer(boost::asio::mutable_buffer buf) : buf_{ std::move(buf) }, optv_{ nullptr} {}
			
			template<Integers T>
			void operator()(const T& i) const {
				*boost::asio::buffer_cast<T*>(buf_) = bswap(i);
				buf_ += sizeof(T);
			}

			void operator()(const boost::uuids::uuid& uuid)  const {
				memcpy(buf_.data(), uuid.data, uuid.static_size());
				buf_ += uuid.static_size();
			}

			void operator()(const std::string& str) const {
				(*this)(static_cast<std::uint32_t>(str.size()));
				memcpy(buf_.data(), str.data(), str.size());
				buf_ += str.size();
			}

			void operator()(const pof::base::currency& cur) const {
				memcpy(buf_.data(), cur.data().data(), cur.data().size());
				buf_ += cur.data().size();
			}
			template<Pods P>
			void operator()(const P& p) const {
				memcpy(buf_.data(), &p, sizeof(P));
				buf_ += sizeof(P);
			}

			template<size_t N>
			void operator()(const std::array<char, N>& fixed) const {
				memcpy(buf_.data(), fixed.data(), fixed.size());
				buf_ += N;
			}

			template<FusionStruct T>
			void operator()(const T& val) const {
				boost::fusion::for_each(val, *this);
			}

			template<typename T, size_t N>
			void operator()(const opt_fields& t) const {
				opt_.reset();
				optv_ = boost::asio::buffer_cast<opt_fields::value_type*>(buf_);
				buf_ += sizeof(opt_fields::value_type);
			}

			template<typename T, size_t N>
			void operator()(const optional_field<T, N>& field) const {
				if (optv_ == nullptr) throw std::logic_error("optional field comes before optional set");
				opt_.set(N);
				*optv_ = static_cast<opt_fields::value_type>(opt_.to_ulong());
				(*this)(*field);
			}
			template<typename T>
				requires Integers<T> || FusionStruct<T> || Enums<T> || Pods<T>
			void operator()(std::vector<T>&vec) const
			{
				(*this)(vec.size());
				for (const auto& v : vec) {
					(*this)(v);
				}
			}
		};

		template<typename T>
		T read(boost::asio::const_buffer b) {
			reader r(std::move(b));
			T res{};
			boost::fusion::for_each(res, r);
			return res;
		}

		template<typename T>
		boost::asio::mutable_buffer write(boost::asio::mutable_buffer buf, const T& val) {
			writer w(std::move(buf));
			boost::fusion::for_each(val, w);
			return w.buf_;
		}
	}
}