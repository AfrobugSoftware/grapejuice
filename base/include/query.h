#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <exception>

#include <boost/signals2.hpp>
#include <boost/mysql.hpp>
#include <boost/noncopyable.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include "Data.h"
#include "errc.h"

//represents data and query read or written to the database
using namespace boost::asio::experimental::awaitable_operators;
namespace pof {
	namespace base {
		template<typename manager>
		struct query : public std::enable_shared_from_this<query<manager>> {
			using default_token = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
			using timer_t = default_token::as_default_on_t<boost::asio::steady_timer>;

			using shared_t = std::enable_shared_from_this<query<manager>>;
			static constexpr auto tuple_awaitable = boost::asio::as_tuple(boost::asio::use_awaitable);

			boost::signals2::signal<void(std::error_code, std::shared_ptr<query>)> m_sig;
			std::string m_sql;
			std::shared_ptr<pof::base::data> m_data;

			boost::mysql::diagnostics m_diag; //server diagonis
			std::shared_ptr<manager> m_manager; //must be the database manager

			query(std::shared_ptr<manager> man = nullptr) : m_manager(man) {
				m_data = std::make_shared<pof::base::data>();
			}

			//Text query
			virtual boost::asio::awaitable<void> operator()() {
				boost::mysql::results result;

				timer_t timer(co_await boost::asio::this_coro::executor);
				timer.expires_after(std::chrono::minutes(1));
				try {
					std::error_code ec;
					auto complete
						= co_await(m_manager->connection().async_execute(m_sql, result, tuple_awaitable)
							|| timer.async_wait());

					switch (complete.index())
					{
					case 0:
						timer.cancel();
						ec = std::get<0>(std::get<0>(complete));
						break;
					case 1:
						//what happens if we timeout ?
						//signal the query on timeout ...
						//ec = std::error_code(boost::asio::error::timed_out);
						ec = boost::system::error_code(boost::asio::error::timed_out);
						break; //
					default:
						break;
					}

					if (ec) {
						//spdlog::error("{:ec}", std::error_code(ec));
						m_sig(ec, shared_t::shared_from_this());
						co_return;
					}
					if (!result.has_value()) {
						//query did not return a value
						//spdlog::info("{:ec}, {}", std::error_code(ec), result.info());
						m_sig(ec, shared_t::shared_from_this());
					}
					else {
						const auto& meta = result.meta();
						auto& datameta = m_data->get_metadata();
						datameta.reserve(meta.size());
						for (const auto& m : meta) {
							auto k = m.type();
							switch (k)
							{
							case boost::mysql::column_type::bigint:
								datameta.emplace_back(pof::base::data::kind::uint64);
								break;
							case boost::mysql::column_type::smallint:
								datameta.emplace_back(pof::base::data::kind::uint32);
								break;
							case boost::mysql::column_type::text:
								datameta.emplace_back(pof::base::data::kind::text);
								break;
							case boost::mysql::column_type::json:
								datameta.emplace_back(pof::base::data::kind::text);
								break;
							case boost::mysql::column_type::double_:
								datameta.emplace_back(pof::base::data::kind::float64);
								break;
							case boost::mysql::column_type::float_:
								datameta.emplace_back(pof::base::data::kind::float32);
								break;
							case boost::mysql::column_type::datetime:
								datameta.emplace_back(pof::base::data::kind::datetime);
								break;
							case boost::mysql::column_type::blob:
								datameta.emplace_back(pof::base::data::kind::float32);
								break;
							case boost::mysql::column_type::int_:
								datameta.push_back(pof::base::data::kind::int32);
								break;
							case boost::mysql::column_type::unknown:
								datameta.push_back(pof::base::data::kind::null);
								break;
							default:
								break;
							}
						}
						const auto& rows = result.rows();
						m_data->reserve(rows.size());
						for (const auto& row : rows) {
							//copy data


						}
						m_sig(ec, shared_t::shared_from_this()); //data moved into the datamodels cache
					}
				}catch (std::exception& exp) {
					auto why = exp.what();
					spdlog::info("{}", why);
			    }
			}
		};

		//Statement queries may require arguments
		template<typename manager, typename... args>
		struct querystmt : public query<manager> {
			using base_t = query<manager>;
			std::tuple<args...> m_arguments;

			querystmt(std::shared_ptr<manager> manager = nullptr) : base_t(manager) {}

			virtual boost::asio::awaitable<void> operator()() override {
				boost::mysql::statement stmt;
				std::error_code ec;
				auto& conn = base_t::m_manager->connection();

				std::tie(ec) = co_await conn.async_prepare_statement(base_t::m_sql, stmt, base_t::m_diag, base_t::tuple_awaitable);
				if (ec) {
					base_t::mSig(ec, base_t::shared_t::shared_from_this());
					co_return;
				}
				boost::mysql::results result;
				std::tie(ec) = co_await conn.async_execute(m_arguments, result, base_t::mDiag, base_t::tuple_awaitable);
				if (ec) {
					base_t::mSig(ec, base_t::shared_t::shared_from_this());
					co_return;
				}
				if (!result.has_value()) {
					base_t::m_sig(ec, base_t::shared_t::shared_from_this());
				}
				else {
					const auto& meta = result.meta();
					auto& datameta = base_t::m_data->get_metadata();
					datameta.reserve(meta.size());
					for (const auto& m : meta) {
						auto k = m.type();
						switch (k)
						{
						case boost::mysql::column_type::bigint:
							datameta.emplace_back(pof::base::data::kind::uint64);
							break;
						case boost::mysql::column_type::smallint:
							datameta.emplace_back(pof::base::data::kind::uint32);
							break;
						case boost::mysql::column_type::text:
							datameta.emplace_back(pof::base::data::kind::text);
							break;
						case boost::mysql::column_type::json:
							datameta.emplace_back(pof::base::data::kind::text);
							break;
						case boost::mysql::column_type::double_:
							datameta.emplace_back(pof::base::data::kind::float64);
							break;
						case boost::mysql::column_type::float_:
							datameta.emplace_back(pof::base::data::kind::float32);
							break;
						case boost::mysql::column_type::datetime:
							datameta.emplace_back(pof::base::data::kind::datetime);
							break;
						case boost::mysql::column_type::blob:
							datameta.emplace_back(pof::base::data::kind::float32);
							break;
						case boost::mysql::column_type::int_:
							datameta.push_back(pof::base::data::kind::int32);
							break;
						case boost::mysql::column_type::unknown:
							datameta.push_back(pof::base::data::kind::null);
							break;
						default:
							break;
						}
					}
					const auto& rows = result.rows();
					base_t::m_data->reserve(rows.size());
					for (const auto& row : rows) {
						//copy data


					}
					base_t::m_sig(ec, base_t::shared_t::shared_from_this()); //data moved into the datamodels cache
				}
			}
		};
	}
};