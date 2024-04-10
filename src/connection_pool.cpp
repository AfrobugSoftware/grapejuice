#include "connection_pool.h"

static std::unique_ptr<filo::conn_pool> g_conn_pool;
static std::once_flag flag;



filo::conn_pool& filo::get_conn_pool(){
	try{
		std::call_once(flag, [&](){
				if(!g_conn_pool){
					g_conn_pool = std::make_unique<filo::conn_pool>(filo::conn_pool::spool_size);
				}
		});
	}catch(std::exception& e){
		spdlog::critical("{}", e.what());
	}

	//cause a crash here if not initialised
	return *g_conn_pool;
}

filo::conn_pool::conn_pool(size_t size): m_pool_size(size){
	while(m_pool.size() < size){
		m_pool.push_back(create()); //add connections to pool;
	}
}


//connection pool id destoryed after main
filo::conn_pool::~conn_pool(){
	if(!m_borrowed.empty()){
		//fatal error, all borrowed connections should be returned before closing
		spdlog::get("err")->critical("{:d} borrowed connections still active on shutdown", m_borrowed.size());
		for(auto& sp: m_borrowed){
			//close connection: 
			m_borrowed.erase(sp);
		}
	}
	if(!m_pool.empty()){
		//pool is not empty, pool connections are not connected
		m_pool.clear();
	}
}
filo::conn_pool::stat filo::conn_pool::stats(){

}

filo::conn_pool::conn_ptr filo::conn_pool::borrow()
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	//no free connections
	if(m_pool.empty()){
		 //are theere any crashed connections listed as borrowed
		for(auto& sp: m_borrowed){
			if(sp.unique()){ 
				//this connection has been abandoned 
				  try{
				  		//co_spawn an ansync close
				  		spdlog::get("output")->info("Creating a new connection to replace old");
				  		auto sp = create();
				  		m_borrowed.erase(sp);
				  		m_borrowed.insert(sp);
				  		return sp;
				  		
				  }catch(std::exception& ec){
				  		throw std::system_error(std::make_error_code(chws::errc::no_connection_avail));
				  }
			}
		}
		//throw inavaliable 
		throw std::system_error(std::make_error_code(chws::errc::no_connection_avail));
	}
	auto sp = m_pool.front();
	m_pool.pop_front();
	m_borrowed.insert(sp);
	return sp;
}


/**
 * Only call this when returing a connection, if the connection is bad
 * allow it go out of scope 
 * */		
void filo::conn_pool::unborrow(filo::conn_pool::conn_ptr conn){
	std::unique_lock<std::mutex> lock(m_mutex);
	m_pool.push_back(conn);
	m_borrowed.erase(conn);

}

void filo::conn_pool::err_callback(conn_ptr ptr, const std::error_code& ec){
	//connecting failed
	spdlog::get("err")->error("Database connection error {} - {}", ec.message(), ec.value());	
	std::unique_lock<std::mutex> lock(m_mutex);
	m_pool.remove(ptr);  //removes from the list, 
}


filo::conn_pool::conn_ptr filo::conn_pool::create(){
    auto& io_context = filo::get_system().iocontext();
    auto& ssl_context = filo::get_system().sslcontext();

    auto sp = std::make_shared<boost::mysql::tcp_ssl_connection>(io_context, ssl_context);
    return sp;
}
