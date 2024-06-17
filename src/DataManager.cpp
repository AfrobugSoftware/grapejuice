#include "DataManager.h"

void grape::DataManager::push(const packBuffer& buffer)
{
	mBuffers.push_back(buffer);
	mHeader.bufferCount++;
}

boost::beast::multi_buffer grape::DataManager::finalise() const
{
	std::vector<std::uint8_t> compressed;
	boost::beast::multi_buffer omb;


	boost::iostreams::filtering_ostream fos;
	fos.push(boost::iostreams::bzip2_compressor());
	fos.push(boost::iostreams::back_inserter(compressed));
	ar::binary_oarchive archive{ fos, flags };

	archive << mHeader.version;
	archive << mHeader.bufferCount;

	for (auto& datum : mBuffers) {
		archive << datum.mName;
		archive << *datum.buffer;
	}

	auto b = omb.prepare(compressed.size());
	boost::asio::buffer_copy(b, boost::asio::buffer(compressed));
	omb.commit(compressed.size());

	return omb;
}

void grape::DataManager::Unpack(const boost::beast::multi_buffer& buffer)
{
	const size_t size = boost::asio::buffer_size(buffer);
	if (size == 0) {
		//is this empty buffer ?
		return;
	}
	pack_t tempbuf(size);
	auto b = buffer.data();
	boost::asio::buffer_copy(boost::asio::buffer(tempbuf), b);

	boost::iostreams::basic_array_source<std::uint8_t> as{ tempbuf.data(), tempbuf.size() };
	boost::iostreams::filtering_istream ifs;
	ifs.push(boost::iostreams::bzip2_decompressor());
	ifs.push(as);

	ar::binary_iarchive archive{ ifs, flags };

	archive >> mHeader.version;
	archive >> mHeader.bufferCount;

	for (size_t i = 0; i < mHeader.bufferCount; i++) {
		packBuffer pb;
		pof::base::data data;

		archive >> pb.mName;
		archive >> data;

		pb.buffer = std::make_shared<pof::base::data>(std::move(data));
		mBuffers.push_back(pb);
	}
}
