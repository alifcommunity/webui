/*
	WebUI Library
	- - - - - - -
	https://github.com/alifcommunity/webui
	Licensed under GNU General Public License v3.0.
	Copyright (C)2020 Hassan DRAGA <https://github.com/hassandraga>.
*/

#define MINIMUM_PACKETS_SIZE (1)
#define TYPE_SIGNATURE 		0xFF
#define TYPE_RUN_JS 		0xFE
#define TYPE_EVENT 			0xFD

// -- Windows ----------------------------------
#ifdef _WIN32
	//Fix: _WIN32_WINNT warning
	#include <SDKDDKVer.h>
#endif

// -- Linux ------------------------------------
// ...

// -- macOS ------------------------------------
// ...

// -- C++ Standard ----------------------------
#include <iostream>		// std::cout / std::cerr
#include <codecvt>		// std::wstring_convert
#include <fstream>		// std::fstream

// -- Boost Web Server Headers -----------------
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

// -- Boost Web Socket Headers -----------------
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>

// -- Boost File System ------------------------
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

// -- Boost Process ----------------------------
#include <boost/process.hpp>
//#include <boost/process/windows.hpp> // windows::hide

// -- JavaScript bridge ------------------------
namespace webui {

	const std::string javascriptbridge = (R"V0G0N(

		// - - - - - - - - - - -
		// WebUI WebSocket
		// - - - - - - - - - - -

		// external const _wssport;
		// external const _wsport;
		// external const _webui_minimum_data;

		var WebUIWS;
		var WebUIWSStatus = false;

		function WebUISocket(){
		
			if ('WebSocket' in window){
			
				WebUIWS = new WebSocket('ws://127.0.0.1:' + _wssport);
				WebUIWS.binaryType = 'blob';
			
				WebUIWS.onopen = function(){

					WebUIWS.binaryType = 'blob';
					WebUIWSStatus = true;
					var _xmlHttp = new XMLHttpRequest();
					_xmlHttp.open('GET', 'http://127.0.0.1:' + _wsport + '/s1', true);
					_xmlHttp.send(null);
					_xmlHttp.open('GET', 'http://127.0.0.1:' + _wsport + '/s2', true);
					_xmlHttp.send(null);
				};

				WebUIWS.onmessage = function (evt) {

					var reader;
					var buffer8;

					reader = new FileReader();
					reader.readAsArrayBuffer(evt.data);
					reader.addEventListener("loadend", function(e){

						buffer8 = new Uint8Array(e.target.result);

						if(buffer8.length < _webui_minimum_data)
							return;
						
						if(buffer8[0] !== 0xFF){
							console.log( 'Error flag: ' + buffer8[0] );
							console.log( 'Error flag: ' + buffer8[1] );
							console.log( 'Error flag: ' + buffer8[2] );
							return;
						}

						console.log( 'Flag OK: ' + buffer8[0] );
						console.log( 'Flag OK: ' + buffer8[1] );
						console.log( 'Flag OK: ' + buffer8[2] );

						var len = buffer8.length - 3;

						data8 = new Uint8Array(len);
						for(i = 0; i < len; i++)
							data8[i] = buffer8[i+3];
						
						var data8utf8 = new TextDecoder("utf-8").decode(data8);

						if(buffer8[1] === 0xFE){

							// var Res = "OK"; // call ...
							// var Res8 = new TextEncoder("utf-8").encode(Res);
							// Res_buffer8 = new Uint8Array(reserved + Res8.length);
							// Res_buffer8[0] = 0xFF;
							// Res_buffer8[1] = buffer8[1];
							// Res_buffer8[2] = buffer8[2];
							// var p = -1;
							// for(i = reserved; i < (reserved + Res8.length); i++)
							// console.log( 'Res_buffer8['+i+'] = Res8[' + (++p) + ']' );

							console.log( 'Run JS: ' + data8utf8 );

							var fun = new Function (data8utf8);
							var FunReturn;
							FunReturn = String(fun());

							if (FunReturn === undefined)
								return;

							console.log( 'Get JS: ' + FunReturn );

							var FunReturn8 = new TextEncoder("utf-8").encode(FunReturn);
							var Return8 = new Uint8Array(3 + FunReturn8.length);
							Return8[0] = 0xFF;
							Return8[1] = 0xFE;
							Return8[2] = buffer8[2];

							var p = -1;
							for(i = 3; i < FunReturn8.length + 3; i++)
								Return8[i] = FunReturn8[++p];
							
							if(Return8[0] !== 0xFF){
								console.log( 'Error response: ' + buffer8[0] );
								console.log( 'Error response: ' + buffer8[1] );
								console.log( 'Error response: ' + buffer8[2] );
								return;
							}

							for(i = 0; i < Return8.length; i++)
								console.log( 'Sending Return8['+ i +']: ' + Return8[i] );

							WebUIWS.send(Return8.buffer);
						}
					});
				};
				
				WebUIWS.onerror = function(){
					window.close();
				};
			
				WebUIWS.onclose = function(){
					window.close();
				};
			}
			else {
				
				alert('Sorry. WebSocket not supported by your Browser.');
				window.close();
			}
		}

		// - - - - - - - - - - -
		// Event
		// - - - - - - - - - - -

		document.onkeydown = function(evt){
			evt = evt || window.event;
			if (evt.keyCode == 116){
				window.close();
			}
		};

		function SendEvent(name){

			if(name != ''){

				var Name8 = new TextEncoder("utf-8").encode(name);
				var Event8 = new Uint8Array(3 + Name8.length);
				Event8[0] = 0xFF;
				Event8[1] = 0xFD;
				Event8[2] = 0x0;

				var p = -1;
				for(i = 3; i < Name8.length + 3; i++)
					Event8[i] = Name8[++p];

				for(i = 0; i < Event8.length; i++)
					console.log( 'Sending Event8['+ i +']: ' + Event8[i] );

				WebUIWS.send(Event8.buffer);
			}
		}

		window.onbeforeunload = function (){
			if(WebUIWSStatus){
				WebUIWS.onclose = function () {}; // disabling onclose handler
				WebUIWS.close();
			}
		};

		window.addEventListener("load",function(){

			var elems = document.getElementsByTagName("form");
			for (i = 0; i < elems.length; i++){
				alert('Incompatible HTML.\n\nYour HTML contain <form> elements, wish is not compatible with WebUI. Please remove all those elements.');
				window.close();
			}
		
			elems = document.getElementsByTagName("button");
			for (i = 0; i < elems.length; i++){
				elems[i].addEventListener("click", function(){ SendEvent(this.id) });
			}
		
			elems = document.getElementsByTagName("div");
			for (i = 0; i < elems.length; i++){
				elems[i].addEventListener("click", function(){ SendEvent(this.id) });
			}
		});

		WebUISocket();
		setTimeout(function(){

			if(!WebUIWSStatus){

				alert('Failed to connect to server');
				window.close();
			}
		}, 3000);

	)V0G0N");
}

// -- Boost namespaces -------------------------
namespace beast = boost::beast;				// from <boost/beast.hpp>
namespace http = beast::http;				// from <boost/beast/http.hpp>
namespace websocket = beast::websocket;		// from <boost/beast/websocket.hpp>
namespace net = boost::asio;				// from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;			// from <boost/asio/ip/tcp.hpp>

namespace webui{

	// -- Global ---
	std::atomic<unsigned short> connected_swindow = 0;
	const std::string html_def = "<html><head><meta http-equiv=\"refresh\" content=\"1\"></head>"
								 "<body style=\"background-color:#515C6B; color:#fff; font-family:\"Lucida Console\", Courier, monospace\">"
								 "Loading..</body></html>";
	std::atomic<bool> waitfor_swindow = false;
	std::vector<std::string> key_v;
	std::array<void(*)(), 64> key_actions;
	unsigned short port_start = 8080;
	std::vector<unsigned short> port_v = { 0 };
	unsigned short winlast = 1;
	unsigned short startup_timeout = 10; // t = n * 1000 / 500
	std::array<std::string *,		64> nat_data;
	std::array<std::atomic<bool>,	64> nat_data_status;
	std::vector<unsigned short>			nat_id_v = { 0 };

	// -- Func ----
	void ini();
	void msgbox(std::wstring msg);
	void msgbox(std::string msg);
	int getkey(std::string id);
	unsigned setkey(const std::string id);
	unsigned short getport();
	unsigned short getwin();
	// void start_webserver(const char * ip, unsigned short port, const std::string * html, webui::_window * ui);
	// void start_websocket(const char * ip, unsigned short port, webui::_window * ui);
	void waitforstartup();
	void FreePort();
	void FreeWindow();
	void loop();
	void exit();

	// -- Class ----
	class _window {
		public:
			_window();
			~_window();			
			struct {
				std::thread * t_ws;
				std::thread * t_wss;
				std::string number_s;
				unsigned short number;
				bool ws_running = false;
				bool wss_running = false;
				bool ws_served = false;
				std::string ws_port = "00000";
				std::string wss_port = "00000";
			} settings;
			void receive(std::vector<std::uint8_t> &packets_v);
			std::string run(std::string js, unsigned short seconds);
			void send(std::vector<std::uint8_t> &packets_v);
			void event(std::string id);
			void websocket_session_clean();
			void bind(std::string key_id, void(*new_action)());
			bool window_show(const std::string * html, unsigned short browser);
			bool window_is_running();
			void destroy();
	};
}

namespace BoostWebServer{

	// - - - - - - - - - - - - -
	// Boost Web Server [small]
	// - - - - - - - - - - - - -

	class http_connection : public std::enable_shared_from_this<http_connection>{

		public:
		const std::string * html_main = &webui::html_def;
		webui::_window *p_ui;

		http_connection(tcp::socket socket)
			: socket_(std::move(socket))
		{
		}

		// Initiate the asynchronous operations associated with the connection.
		void
		start(const std::string * html, webui::_window * ui)
		{
			this->html_main = html;
			this->p_ui = ui;
			
			read_request();
			check_deadline();
		}

		private:
		// The socket for the currently connected client.
		tcp::socket socket_;

		// The buffer for performing reads.
		beast::flat_buffer buffer_{8192};

		// The request message.
		http::request<http::dynamic_body> request_;

		// The response message.
		http::response<http::dynamic_body> response_;

		// The timer for putting a deadline on connection processing.
		net::steady_timer deadline_{
			socket_.get_executor(), std::chrono::seconds(60)};

		// Asynchronously receive a complete request message.
		void
		read_request()
		{
			auto self = shared_from_this();
			
			http::async_read(
				socket_,
				buffer_,
				request_,
				[self](beast::error_code ec,
					std::size_t bytes_transferred)
				{
					boost::ignore_unused(bytes_transferred);
					if(!ec)
						self->process_request();
				});
		}

		// Determine what needs to be done with the request message.
		void
		process_request()
		{
			response_.version(request_.version());
			response_.keep_alive(false);

			switch(request_.method())
			{
				case http::verb::get:
				response_.result(http::status::ok);
				response_.set(http::field::server, "WebUI");
				create_response();
				break;

				default:
				// We return responses indicating an error if
				// we do not recognize the request method.
				response_.result(http::status::bad_request);
				response_.set(http::field::server, "WebUI");
				response_.set(http::field::content_type, "text/plain");
				beast::ostream(response_.body())
					<< "Invalid request-method '"
					<< std::string(request_.method_string())
					<< "'";
				break;
			}

			write_response();
		}

		// Construct a response message based on the program state.
		void
		create_response()
		{
			this->p_ui->settings.ws_served = true;

			response_.set(http::field::content_type, "text/html; charset=utf-8");
			beast::ostream(response_.body())	<< *this->html_main 
												<< "\n <script type = \"text/javascript\"> \n const _wssport = " 
												<< this->p_ui->settings.wss_port
												<< "; \n "
												<< " const _wsport = " 
												<< this->p_ui->settings.ws_port
												<< "; \n "
												<< " const _webui_minimum_data = " 
												<< MINIMUM_PACKETS_SIZE
												<< "; \n "
												<< webui::javascriptbridge
												<< " \n </script>";
		}

		// Asynchronously transmit the response message.
		void
		write_response()
		{
			auto self = shared_from_this();

			response_.content_length(response_.body().size());

			http::async_write(
				socket_,
				response_,
				[self](beast::error_code ec, std::size_t)
				{
					self->socket_.shutdown(tcp::socket::shutdown_send, ec);
					self->deadline_.cancel();
				});
		}

		// Check whether we have spent enough time on this connection.
		void
		check_deadline()
		{
			auto self = shared_from_this();

			deadline_.async_wait(
				[self](beast::error_code ec)
				{
					if(!ec)
					{
						// Close socket to cancel any outstanding operation.
						self->socket_.close(ec);
					}
				});
		}
	};

	// -- HTTP loop ------------------------------------------
	class http_loop {

		public:
		const std::string * html_def = &webui::html_def;
		webui::_window *p_ui;

		void
		http_server(tcp::acceptor& acceptor, tcp::socket& socket){
			
			if(this->p_ui->settings.ws_served)
				return;

			acceptor.async_accept(socket,
			[&](beast::error_code ec)
			{
				if(!ec)
					std::make_shared<http_connection>(std::move(socket))->start(this->html_def, this->p_ui);

				http_server(acceptor, socket);
			});
		}
	};
}

namespace BoostWebSocket{

	// - - - - - - - - - - - - - - - -
	// Boost Web Socket Server [async]
	// - - - - - - - - - - - - - - - -

	class session : public std::enable_shared_from_this<session>{

		websocket::stream<beast::tcp_stream> ws_;
		beast::flat_buffer buffer_;

		private:

		void fail(beast::error_code ec, char const* what){

			std::cerr << "[!] {session}: " << what << ": " << ec.message() << "\n";

			// Close the WebSocket connection
			ws_.async_close(websocket::close_code::normal,
				beast::bind_front_handler(
					&session::on_close,
					shared_from_this()
				)
			);
		}

		public:

		webui::_window *p_ui;

		~session(){

			//printf("~session() \n");
		}

		// Take ownership of the socket
		explicit session(tcp::socket&& socket) : ws_(std::move(socket)){

			ws_.binary(true);
		}

		// Get on the correct executor
		void run(webui::_window * ui);

		// Start the asynchronous operation
		void
		on_run()
		{
			ws_.binary(true);

			// Set suggested timeout settings for the websocket
			ws_.set_option(
				websocket::stream_base::timeout::suggested(
					beast::role_type::server));

			// Set a decorator to change the Server of the handshake
			ws_.set_option(websocket::stream_base::decorator(
				[](websocket::response_type& res)
				{
					// res.set(http::field::server,
					// 	const std::string * (BOOST_BEAST_VERSION_STRING) +
					// 		" websocket-server-async");
					res.set(http::field::server,
						"WebUI");
				}));

			// Accept the websocket handshake
			ws_.async_accept(
				beast::bind_front_handler(
					&session::on_accept,
					shared_from_this()));
		}

		void
		on_accept(beast::error_code ec)
		{
			if(ec)
				return fail(ec, "accept");

			// Read a message
			do_read();
		}

		void
		do_read()
		{
			// Read a message into our buffer
			ws_.async_read(
				buffer_,
				beast::bind_front_handler(
					&session::on_read,
					shared_from_this()));
		}

		void receive_task(std::vector<std::uint8_t> &packets_v){	

			this->p_ui->receive(packets_v);
		}

		void
		on_read(
			beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			// This indicates that the session was closed
			if(ec == websocket::error::closed)
				return;

			if(ec)
				fail(ec, "read");
			
			if (ws_.got_binary()){
				
				std::vector<std::uint8_t> packets_v;
				
				// Loop cast way
				// packets_v.reserve(buffer_.data().size());
				// net::const_buffer cb = buffer_.data();
				// for(std::size_t i = 0; i < cb.size(); ++i){
				// 	//packets_v.push_back( reinterpret_cast<unsigned char const*>(cb.data())[i] );
				// 	packets_v.push_back( reinterpret_cast<std::uint8_t const*>(cb.data())[i] );
				// }

				// Net buffer copy way
				auto cb = buffer_.data();
				packets_v.resize(beast::buffer_bytes(cb));
				buffer_.consume(net::buffer_copy(net::buffer(packets_v), cb));

				if(packets_v[0] != TYPE_SIGNATURE)
					return;
				
				if(packets_v.size() < MINIMUM_PACKETS_SIZE)
					return;
				
				// -- Process message --------------------------------------------
				std::thread receive_job(&session::receive_task, this, packets_v);
				receive_job.detach();
				// ---------------------------------------------------------------
			}

			// Clear buffer
			buffer_.consume(buffer_.size());

			ws_.async_write(
				buffer_.data(),
				beast::bind_front_handler(
					&session::on_write,
					shared_from_this()
				)
			);
		}

		void
		on_write(
			beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			if(ec)
				return fail(ec, "write");

			// Clear the buffer
			buffer_.consume(buffer_.size());

			// Do another read
			do_read();
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		void send(std::vector<std::uint8_t> &packets_v){

			if(packets_v.size() < MINIMUM_PACKETS_SIZE)
				return;

			ws_.write(boost::asio::buffer(packets_v, packets_v.size()));
			
			// Clear buffer?
			//buffer_.consume(buffer_.size());
		}

		void on_close(beast::error_code ec){

			std::cout << "[!] on_close(). " << std::endl;
		}
	};

	std::vector<BoostWebSocket::session> socket_session_v;

	void BoostWebSocket::session::run(webui::_window * ui){

		this->p_ui = ui;

		// We need to be executing within a strand to perform async operations
		// on the I/O objects in this session. Although not strictly necessary
		// for single-threaded contexts, this example code is written to be
		// thread-safe by default.
		net::dispatch(ws_.get_executor(),
			beast::bind_front_handler(
				&session::on_run,
				shared_from_this()));
	}

	// Accepts incoming connections 
	// and launches the sessions
	class listener : public std::enable_shared_from_this<listener>
	{
		net::io_context& ioc_;
		tcp::acceptor acceptor_;

		private:

		void fail(beast::error_code ec, char const* what){

			std::cerr << "[!] {listener}: " << what << ": " << ec.message() << "\n";
			
			// Close the IOC service
			// ...
		}

		public:
		bool *p_WindowWasConnected = false;
		webui::_window *p_ui;

		listener(
			net::io_context& ioc,
			tcp::endpoint endpoint)
			: ioc_(ioc)
			, acceptor_(ioc)
		{
			beast::error_code ec;

			// Open the acceptor
			acceptor_.open(endpoint.protocol(), ec);
			if(ec)
			{
				fail(ec, "open");
				return;
			}

			// Allow address reuse
			acceptor_.set_option(net::socket_base::reuse_address(true), ec);
			if(ec)
			{
				fail(ec, "set_option");
				return;
			}

			// bind to the server address
			acceptor_.bind(endpoint, ec);
			if(ec)
			{
				fail(ec, "bind");
				return;
			}

			// Start listening for connections
			acceptor_.listen(
				net::socket_base::max_listen_connections, ec);
			if(ec)
			{
				fail(ec, "listen");
				return;
			}
		}

		// Start accepting incoming connections
		void
		run(bool * WindowWasConnected, webui::_window * ui)
		{
			this->p_WindowWasConnected = WindowWasConnected;
			this->p_ui = ui;

			do_accept();
		}

		private:
		void
		do_accept()
		{
			// The new connection gets its own strand
			acceptor_.async_accept(
				net::make_strand(ioc_),
				beast::bind_front_handler(
					&listener::on_accept,
					shared_from_this()
				)
			);
		}

		void on_accept(beast::error_code ec, tcp::socket socket);
	};
}

namespace webui{

	std::array<std::shared_ptr<BoostWebSocket::session>, 64> session_actions;	
}

namespace BoostWebSocket{

	void listener::on_accept(beast::error_code ec, tcp::socket socket){

			if(ec){

				fail(ec, "accept");
			}
			else {

				webui::connected_swindow++;
				this->p_ui->settings.wss_running = true;
				*this->p_WindowWasConnected = true;

				// Create the session and run it
				//std::make_shared<session>(std::move(socket) )-> run(this->p_ui);

				std::shared_ptr<BoostWebSocket::session> p_session = std::make_shared<BoostWebSocket::session>(std::move(socket));
				p_session->run(this->p_ui);

				webui::session_actions[this->p_ui->settings.number] = p_session;
			}

			// Accept another connection
			//do_accept();
	}
}

namespace webui{

	namespace browser{

		#define chrome		(1)
		#define firefox		(2)
		#define edge		(3)
		#define safari		(3)

		unsigned short CurrentBrowser = 0;
		std::string browser_path;
		std::string frofile_path;

		int command(std::string cmd){

			//boost::process::environment env = ::boost::this_process::environment();
			//boost::process::child c(cmd, browser::env, boost::process::windows::hide);
			boost::process::child c(cmd);

			c.wait();
			return c.exit_code();
		}

		int command_browser(std::string cmd){

			boost::process::child c(cmd);
			c.detach();
			return 0;
		}

		void clean(){

			if(browser::CurrentBrowser == chrome)
				browser::command("if exist \"%tmp%\\WebUIChromeProfile\" rmdir \"%tmp%\\WebUIChromeProfile\" /s /q");
		}

		bool browserr_exist(unsigned short browser){

			#ifdef _WIN32
				char* p_drive = std::getenv("SystemDrive");
				if(p_drive == nullptr)
					return false;
				std::string drive = p_drive;
				std::string programs_folder32 = drive + "\\Program Files (x86)";
				std::string programs_folder64 = drive + "\\Program Files";
			#elif __APPLE__
				std::string programs_folder = "/Applications";
			#else
				std::string programs_folder = "/usr/bin";
			#endif

			if(browser == firefox){

				// Firefox

				#ifndef _WIN32
					std::string fullpath;
				#endif

				#ifdef _WIN32

					// Firefox 32/64 on Windows

					std::string fullpath32 = programs_folder32 + "\\Mozilla Firefox\\firefox.exe";
					std::string fullpath64 = programs_folder64 + "\\Mozilla Firefox\\firefox.exe";

					if(boost::filesystem::is_regular_file(fullpath64)){

						browser_path = "\"" + fullpath64 + "\"";
						return true;
					}
					else if(boost::filesystem::is_regular_file(fullpath32)){

						browser_path = "\"" + fullpath32 + "\"";
						return true;
					}
					else return false;

				#elif __APPLE__
					// Firefox on macOS
					std::string fullpath = "\"" + programs_folder + "/Mozilla Firefox/firefox.app/.../...\"";
				#else
					// Firefox on Linux
					std::string fullpath = "\"" + programs_folder + "/firefox\"";
				#endif

				#ifndef _WIN32
					// Firefox on macOS/Linux
					if(boost::filesystem::is_regular_file(fullpath)){
						browser_path = fullpath;
						return true;
					}					
				#endif
			}
			else if(browser == chrome){

				// Chrome

				#ifndef _WIN32
					std::string fullpath;
				#endif

				#ifdef _WIN32

					// Chrome on Windows

					std::string fullpath32 = programs_folder32 + "\\Google\\Chrome\\Application\\chrome.exe";
					std::string fullpath64 = programs_folder64 + "\\Google\\Chrome\\Application\\chrome.exe";

					if(boost::filesystem::is_regular_file(fullpath64)){

						browser_path = "\"" + fullpath64 + "\"";
						return true;
					}
					else if(boost::filesystem::is_regular_file(fullpath32)){

						browser_path = "\"" + fullpath32 + "\"";
						return true;
					}
					else return false;

				#elif __APPLE__
					// Chrome on macOS
					std::string fullpath = "\"" + programs_folder + "/Google/Chrome/Application/chrome.app/.../...\"";
				#else
					// Chrome on Linux
					std::string fullpath = "\"" + programs_folder + "/chrome\"";
				#endif

				#ifndef _WIN32
					// Chrome on macOS/Linux
					if(boost::filesystem::is_regular_file(fullpath)){
						browser_path = fullpath;
						return true;
					}					
				#endif
			}
			else if(browser == edge){

				// Edge

				#ifndef _WIN32
					std::string fullpath;
				#endif

				#ifdef _WIN32

					// Edge on Windows

					std::string fullpath32 = programs_folder32 + "\\Microsoft\\Edge\\Application\\msedge.exe";
					std::string fullpath64 = programs_folder64 + "\\Microsoft\\Edge\\Application\\msedge.exe";

					if(boost::filesystem::is_regular_file(fullpath64)){

						browser_path = "\"" + fullpath64 + "\"";
						return true;
					}
					else if(boost::filesystem::is_regular_file(fullpath32)){

						browser_path = "\"" + fullpath32 + "\"";
						return true;
					}
					else return false;

				#elif __APPLE__
					// Edge on macOS
					std::string fullpath = "\"" + programs_folder + "/Microsoft/Edge/Application/msedge.app/.../...\"";
				#else
					// Edge on Linux
					std::string fullpath = "\"" + programs_folder + "/Edge\"";
				#endif

				#ifndef _WIN32
					// Edge on macOS/Linux
					if(boost::filesystem::is_regular_file(fullpath)){
						browser_path = fullpath;
						return true;
					}					
				#endif
			}

			return false;
		}

		std::string get_temp_path(){

			boost::filesystem::path p(boost::filesystem::temp_directory_path());
			return p.string();
		}

		bool folder_exist(std::string folder){

			boost::filesystem::path p(folder);
			return boost::filesystem::is_directory(p);
		}

		bool create_profile_folder(unsigned short browser){

			std::string temp = get_temp_path();

			if(browser == chrome){

				frofile_path = temp + "\\WebUIChromeProfile";
				return true;
			}
			
			if(browser == edge){

				frofile_path = temp + "\\WebUIEdgeProfile";
				return true;
			}

			std::string profile_name = "";

			if(browser == firefox)
				profile_name = "WebUIFirefoxProfile";

			#ifdef _WIN32

				if(!folder_exist(temp + "\\" + profile_name)){

					browser::command(browser::browser_path + " -CreateProfile \"WebUI " + temp + "\\" + profile_name + "\"");

					for(unsigned short n = 0; n <= 10; n++){

						if(folder_exist(temp + "\\" + profile_name))
							break;
						
						std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::milliseconds(500));
					}

					if(!folder_exist(temp + "\\" + profile_name))
						return false;

					// prefs.js
					std::fstream prefs(temp + "\\" + profile_name + "\\prefs.js", std::ios::out | std::ios::app);
						if(prefs.fail())
							return false;
						prefs << "user_pref(\"toolkit.legacyUserProfileCustomizations.stylesheets\", true); " << std::endl;
						prefs << "user_pref(\"browser.shell.checkDefaultBrowser\", false); " << std::endl;
						prefs << "user_pref(\"browser.tabs.warnOnClose\", false); " << std::endl;
					prefs.close();

					// userChrome.css
					boost::filesystem::create_directory(temp + "\\" + profile_name + "\\chrome\\");
					std::fstream userChrome(temp + "\\" + profile_name + "\\chrome\\userChrome.css", std::ios::out | std::ios::app);
						if(prefs.fail())
							return false;
						userChrome << ":root{--uc-toolbar-height:32px}:root:not([uidensity=\"compact\"]){--uc-toolbar-height:38px}#TabsToolbar{visibility:collapse!important}:root:not([inFullscreen]) #nav-bar{margin-top:calc(0px - var(--uc-toolbar-height))}#toolbar-menubar{min-height:unset!important;height:var(--uc-toolbar-height)!important;position:relative}#main-menubar{-moz-box-flex:1;background-color:var(--toolbar-bgcolor,--toolbar-non-lwt-bgcolor);background-clip:padding-box;border-right:30px solid transparent;border-image:linear-gradient(to left,transparent,var(--toolbar-bgcolor,--toolbar-non-lwt-bgcolor) 30px) 20 / 30px}#toolbar-menubar:not([inactive]){z-index:2}#toolbar-menubar[inactive] > #menubar-items{opacity:0;pointer-events:none;margin-left:var(--uc-window-drag-space-width,0px)}#nav-bar{visibility:collapse}" << std::endl;
					userChrome.close();

					frofile_path = temp + "\\" + profile_name;
				}

			#elif __APPLE__
				// ...
			#else
				// ...
			#endif

			return true;
		}

		bool start_edge(unsigned short port){

			// -- Edge ----------------------

			if(CurrentBrowser != 0 && CurrentBrowser != edge)
				return false;

			if(!browserr_exist(edge))
				return false;
			
			if(!create_profile_folder(edge))
				return false;

			std::string arg = " --user-data-dir=\"" + frofile_path + "\" --no-proxy-server --app=http://127.0.0.1:";	
			std::string s_port = std::to_string(port);
			std::string full(browser::browser_path);
			full.append(arg);
			full.append(s_port);

			if(browser::command_browser(full) == 0){

				browser::CurrentBrowser = edge;
				return true;
			}
			else
				return false;
		}

		bool start_firefox(unsigned short port){

			// -- Firefox ----------------------

			if(CurrentBrowser != 0 && CurrentBrowser != firefox)
				return false;

			if(!browserr_exist(firefox))
				return false;
			
			if(!create_profile_folder(firefox))
				return false;

			std::string arg = " -P WebUI -private -no-remote -new-instance http://127.0.0.1:";	
			std::string s_port = std::to_string(port);
			std::string full(browser::browser_path);
			full.append(arg);
			full.append(s_port);

			if(browser::command_browser(full) == 0){

				browser::CurrentBrowser = firefox;
				return true;
			}
			else
				return false;
		}

		bool start_chrome(unsigned short port){

			// -- Chrome ----------------------

			if(CurrentBrowser != 0 && CurrentBrowser != chrome)
				return false;

			if(!browserr_exist(chrome))
				return false;
			
			if(!create_profile_folder(chrome))
				return false;

			std::string arg = " --user-data-dir=\"" + frofile_path + "\" --no-proxy-server --app=http://127.0.0.1:";	
			std::string s_port = std::to_string(port);
			std::string full(browser::browser_path);
			full.append(arg);
			full.append(s_port);

			if(browser::command_browser(full) == 0){

				browser::CurrentBrowser = chrome;
				return true;
			}
			else
				return false;
		}

		bool start(unsigned short port, unsigned short browser){

			if(browser != 0){

				// Specified browser
				if(browser == chrome)
					return start_chrome(port);
				else if(browser == firefox)
					return start_firefox(port);
				else if(browser == edge)
					return start_edge(port);
				else
					return false;
			}
			else if(CurrentBrowser != 0){

				// Already set browser
				if(CurrentBrowser == chrome)
					return start_chrome(port);
				else if(CurrentBrowser == firefox)
					return start_firefox(port);
				else if(CurrentBrowser == edge)
					return start_edge(port);
				else
					return false;
					//webui::exit();
			}
			else {

				// Default system browser

				#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
					// Windows
					if(!start_chrome(port))
						if(!start_firefox(port))
							if(!start_edge(port))
								return false;
								//webui::exit();
				#elif __APPLE__
					// macOS
					if(!start_chrome(port))
						if(!start_firefox(port))
							if(!start_edge(port))
								return false;
								//webui::exit();
				#else
					// Linux
					if(!start_chrome(port))
						if(!start_firefox(port))
							if(!start_edge(port))
								return false;
								//webui::exit();
				#endif
			}

			return true;
		}
	}

	static std::atomic<bool> initialized = false;

	void ini(){

		if(webui::initialized)
			return;
		webui::initialized = true;

		webui::key_v.push_back("");
		webui::port_v.clear();
	}

	void msgbox(std::wstring msg){

		std::wcout << msg << std::endl;
		return;

		#ifdef _WIN32
			LPCWSTR wide_msg;
			wide_msg = msg.c_str();
			MessageBox(NULL, wide_msg, L"Message", NULL);
		#else
			wprintf(L"%s \n", msg);
		#endif
	}

	void msgbox(std::string msg){

		std::cout << msg << std::endl;
		return;

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide = converter.from_bytes(msg);

		msgbox(wide);
	}

	int getkey(std::string id){

		for (int i = 0; i < webui::key_v.size(); i++){

			if (webui::key_v[i] == id){

				return i;
			}
		}

		return -1;
	}

	unsigned setkey(const std::string id){

		webui::key_v.push_back(id);
		return webui::key_v.size() - 1;
	}

	void waitforstartup(){

		if(webui::connected_swindow > 0)
			return;

		for(unsigned short n = 0; n <= (webui::startup_timeout * 2); n++){

			if(webui::connected_swindow > 0)
				break;
			
			std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::milliseconds(500));
		}
	}

	void loop(){

		if(browser::CurrentBrowser < 1)
			return;

		if(webui::waitfor_swindow){

			// Wait one time while user 
			// browser is starting!
			webui::waitforstartup();

			while(webui::connected_swindow > 0){
			
				std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::milliseconds(100));
			}
		}

		// Keep WebUI profile to 
		// save _window positions
		//browser::clean();
	}

	void exit(){

		webui::waitfor_swindow = false;
		webui::connected_swindow = 0;
	}

	void FreePort(unsigned short p){

		webui::port_v.erase(std::remove(webui::port_v.begin(), webui::port_v.end(), p), webui::port_v.end());
	}

	void FreeWindow(){

		webui::connected_swindow--;

		if(webui::connected_swindow < 1)
			webui::waitfor_swindow = false;
	}

	bool port_is_free(unsigned short port_num){

		// Step 1.
		// ...
		
		// Step 2. Creating an endpoint.
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port_num);

		// Used by 'acceptor' class constructor.
		boost::asio::io_service ios;

		// Step 3. Creating and opening an acceptor socket.
		boost::asio::ip::tcp::acceptor acceptor(ios, ep.protocol());

		// Step 4. Binding the acceptor socket.
		boost::system::error_code ec;
		acceptor.bind(ep, ec);

		// Handling errors if any.
		if (ec)
			return false;
		
		return true;
	}

	unsigned short getport(){
		
		unsigned short p = webui::port_start;

		for(unsigned short i = 1; i <= 65535; i++){

			if(std::find(webui::port_v.begin(), webui::port_v.end(), p) != webui::port_v.end())
				// Port used by local window
				p++;
			else {
				if(port_is_free(p))
					// Port ready to use
					break;
				else
					// Port used by an external app
					p++;
			}
		}

		//msgbox(std::to_string(p));

		webui::port_v.push_back(p);
		
		return p;
	}

	unsigned short getwin(){
		
		return webui::winlast++;
	}

	void start_webserver(const char * ip, unsigned short port, const std::string * html, webui::_window * ui){

		// Initialization
		ui->settings.ws_running = true;
		const std::string s_port = std::to_string(port);
		ui->settings.ws_port = s_port;

		try {

			// IO Context
			auto const address = net::ip::make_address(ip);
			net::io_context ioc{1}; // 1 thread
			tcp::acceptor acceptor{ioc, {address, port}};
			tcp::socket socket{ioc};
			BoostWebServer::http_loop Server;
			Server.html_def = html;
			Server.p_ui = ui;
			Server.http_server(acceptor, socket);
			ioc.run();
		}
		catch(std::exception const& e){

			webui::msgbox(L"Error.\nWebUI failed to start http web server.");
			std::cerr << "[!] start_webserver(): " << e.what() << std::endl;
		}

		ui->settings.ws_running = false;
		ui->settings.ws_served = false;
		webui::FreePort(port);
	}

	void start_websocket(const char * ip, unsigned short port, webui::_window * ui){

		// Initialization
		bool WindowWasConnected = false;
		std::string s_port = std::to_string(port);
		ui->settings.wss_port = s_port;
	
		// IO Context
		auto const address = net::ip::make_address(ip);
		auto const threads = std::max<int>(1, 1); // 1 thread
		net::io_context ioc;
		std::make_shared<BoostWebSocket::listener>(ioc, tcp::endpoint{address, port})->run(&WindowWasConnected, ui);
		ioc.run();

		// Clean
		if(WindowWasConnected)
			webui::FreeWindow();
		webui::session_actions[ui->settings.number] = nullptr;	
		webui::FreePort(port);
		ui->settings.wss_running = false;
	}

	// --- Window ------------------------------------

	_window::_window(){

		this->settings.t_ws = nullptr;
		this->settings.t_wss = nullptr;
		this->settings.number = webui::getwin();
		this->settings.number_s = std::to_string(this->settings.number);
		this->settings.ws_running = false;
		this->settings.ws_served = false;
		this->settings.wss_running = false;
		this->settings.ws_port = "00000";
		this->settings.wss_port = "00000";
	}

	_window::~_window(){

		//printf("~_window() \n");
	}		

	void _window::destroy(){

		// ...
	}

	bool _window::window_is_running(){

		return this->settings.wss_running;
	}

	unsigned short get_nat_id(){
		
		unsigned short id = 0;

		for(unsigned short i = 0; i <= 1024; i++){

			if(std::find(webui::nat_id_v.begin(), webui::nat_id_v.end(), id) != webui::nat_id_v.end()){

				id++;
			}
			else break;
		}

		webui::nat_id_v.push_back(id);
		
		return id;
	}
	
	void free_nat_id(unsigned short id){

		webui::nat_data[id] = nullptr;
		webui::nat_id_v.erase(std::remove(webui::nat_id_v.begin(), webui::nat_id_v.end(), id), webui::nat_id_v.end());
	}

	void _window::send(std::vector<std::uint8_t> &packets_v){

		if(packets_v.size() < MINIMUM_PACKETS_SIZE)
			return;

		if(webui::session_actions[this->settings.number] && webui::session_actions[this->settings.number] != nullptr)
			webui::session_actions[this->settings.number]->send(packets_v);

			// TODO: make send(const packets_v)
			// TODO: add custom quee system. dont call send() if busy.
	}

	std::string _window::run(std::string js, unsigned short seconds){

		if(js.empty())
			return "";

		// Initializing pipe
		std::string buf_pipe;
		unsigned short id = webui::get_nat_id();
		webui::nat_data_status[id] = false;
		webui::nat_data[id] = &buf_pipe;

		// Prepare packets
		std::vector<std::uint8_t> packets_v;
		packets_v.push_back(TYPE_SIGNATURE);						// Signature
		packets_v.push_back(TYPE_RUN_JS);							// Type
		packets_v.push_back(id);									// ID
		packets_v.insert(packets_v.end(), js.begin(), js.end());	// Data

		// Send packets
		this->send(packets_v);
		
		for(unsigned short n = 0; n <= (seconds * 100); n++){

			if(webui::nat_data_status[id])
				break;
			
			std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::milliseconds(10));
		}

		// Clean
		webui::free_nat_id(id);
		return buf_pipe;
	}

	void _window::receive(std::vector<std::uint8_t> &packets_v){

		if(packets_v[0] != TYPE_SIGNATURE)
			return;
		
		if(packets_v.size() < MINIMUM_PACKETS_SIZE)
			return;

		std::uint8_t type	= packets_v[1];
		std::uint8_t id		= packets_v[2];

		// Clear first 3 elemets
		packets_v.erase(packets_v.cbegin(), packets_v.cbegin() + 3);

		std::string data(packets_v.begin(), packets_v.end());

		if (type == TYPE_EVENT){

			// Event

			if(!data.empty())
				this->event(this->settings.number_s + "/" + data);
		}
		else if (type == TYPE_RUN_JS){

			// JS Result

			if(id < 0)
				return;
			
			if(webui::nat_data[id] == nullptr)
				return;

			*webui::nat_data[id] = data;
			webui::nat_data_status[id] = true;
		}
	}

	void _window::websocket_session_clean(){

		webui::session_actions[this->settings.number] = nullptr;
	}

	void _window::event(std::string id){

		int key_id = webui::getkey(id);

		if(key_id >= 0)
			if(webui::key_actions[key_id])
				webui::key_actions[key_id]();
	}

	void _window::bind(std::string key_id, void(*new_action)()){

		key_id = this->settings.number_s + "/" + key_id;

		int old_id = webui::getkey(key_id);
		if(old_id >= 0){

			// Replace a reference
			webui::key_actions[old_id] = new_action;
		}
		else {

			// New reference
			webui::key_actions[setkey(key_id)] = new_action;
		}
	}

	bool _window::window_show(const std::string * html, unsigned short browser){

		// window app loop mode
		webui::waitfor_swindow = true;

		unsigned short port_web = webui::getport();
		unsigned short port_ws = webui::getport();

		if(!browser::start(port_web, browser))
			return false;
		
		// -- Web Server ----------------------------
		std::thread StartWebServer_job(	&webui::start_webserver,	// pointer
										"127.0.0.1",				// IP
										port_web,					// Port
										html,						// HTML Code
										this);						// obj
		this->settings.t_ws = &StartWebServer_job;
		StartWebServer_job.detach();
		// ------------------------------------------

		// -- Web Socket ----------------------------
		std::thread StartWebSocket_job(	&webui::start_websocket,	// pointer
										"127.0.0.1",				// IP
										port_ws,					// Port
										this);						// obj
		this->settings.t_wss = &StartWebSocket_job;
		StartWebSocket_job.detach();
		// ------------------------------------------

		return true;
	}
}