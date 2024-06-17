#pragma once
#include "Data.h"
#include "packages.h"

#include <ranges>
#include <list>
#include <vector>

#include <boost/beast/core/multi_buffer.hpp>

namespace grape {
	class DataManager {
	public:
		using pack_t = std::vector<std::uint8_t>;
		struct Header{
			std::uint8_t version = 0x00;
			std::uint32_t bufferCount = 0;
		};

		struct packBuffer{
			std::array<std::uint8_t, 256> mName = {0};
			std::shared_ptr<pof::base::data> buffer;
		};

		DataManager() = default;
		~DataManager() = default;
		void push(const packBuffer& buffer);
		boost::beast::multi_buffer finalise() const;  //returns the multibuffer containing all the buffers
		void Unpack(const boost::beast::multi_buffer& buffer);


		const std::list<packBuffer>& GetBuffers() const { return mBuffers; }
	private:
		static constexpr auto const flags = boost::archive::no_header | boost::archive::no_tracking | boost::archive::no_xml_tag_checking;
		Header mHeader;
		std::list<packBuffer> mBuffers;

	};
}


