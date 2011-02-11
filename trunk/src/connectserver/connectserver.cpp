#include <iostream>
#include <signal.h>
#include "connectserver.h"

#pragma warning(disable: 4355)

bool connectServer_t::m_interrupt = false;

connectServer_t::connectServer_t(size_t maxStoredLogsCount,
									size_t userMax,
									unsigned short port):
  m_userManager(userMax),
  m_logger("log\\logs.html",
			maxStoredLogsCount),
  m_scheduler(m_synchronizer),
  m_iocpEngine(m_logger,
				m_synchronizer),
  m_tcpServer(m_logger,
				m_iocpEngine,
				boost::bind(&connectServer_t::onContextAllocate, this),
				port),
  m_udpSocket(m_logger,
				boost::bind(&connectServer_t::onDatagramReceive, this, _1, _2, _3),
				port),
  m_game(m_logger,
			m_scheduler,
			m_protocol,
			boost::bind(&connectServer_t::disconnect, this, _1)),
  m_protocol(m_game,
				boost::bind(&connectServer_t::send, this, _1, _2)),
  m_userCount(0) {}

void connectServer_t::startup() {
	m_logger.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Starting game.";
	m_logger.out();
	m_game.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Starting iocp engine.";
	m_logger.out();
	m_iocpEngine.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Starting user manager.";
	m_logger.out();
	m_userManager.startup(boost::bind(&connectServer_t::onContextAttach, this, _1),
							boost::bind(&connectServer_t::onContextReceive, this, _1),
							boost::bind(&connectServer_t::onContextClose, this, _1));

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Starting tcp server on port " << m_tcpServer.getListenPort() << ".";
	m_logger.out();
	m_tcpServer.startup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Starting udp socket on port " << m_udpSocket.getPort() << ".";
	m_logger.out();
	m_udpSocket.startup();

	this->updateWindowTitle();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Waiting for connections.";
	m_logger.out();
}

void connectServer_t::cleanup() {
	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Disconnecting all users.";
	m_logger.out();
	this->disconnectAll();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Cleaning udp socket.";
	m_logger.out();
	m_udpSocket.cleanup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Cleaning tcp server.";
	m_logger.out();
	m_tcpServer.cleanup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Cleaning iocp engine.";
	m_logger.out();
	m_iocpEngine.cleanup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Cleaning user manager.";
	m_logger.out();
	m_userManager.cleanup();

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << "Cleaning logger.";
	m_logger.out();
	m_logger.cleanup();
}

void connectServer_t::worker() {
	while(!m_interrupt) {
		m_udpSocket.worker();
		m_scheduler.worker();
		Sleep(1);
	}
}

void connectServer_t::updateWindowTitle() const {
	std::stringstream titleStream;
	titleStream << "[ConnectServer] ::"
				<< " [Users: " << m_userCount << "/" << m_userManager.getCount() << "] ::"
				<< " [Port: " << m_tcpServer.getListenPort() << "]";

	SetConsoleTitle(titleStream.str().c_str());
}

void connectServer_t::disconnectAll() {
	for(size_t i = 0; i < m_userManager.getCount(); ++i) {
		if(m_userManager[i].isActive()) {
			this->disconnect(m_userManager[i]);
		}
	}
}

connectServerUser_t* connectServer_t::onContextAllocate() {
	connectServerUser_t *user = m_userManager.findFree();

	if(user == NULL) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "No free objects found.";
		m_logger.out();
	}

	return user;
}

void connectServer_t::onContextAttach(eMUCore::socketContext_t &context) {
	connectServerUser_t &user = reinterpret_cast<connectServerUser_t&>(context);

	try {
		m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Connected.";
		m_logger.out();

		// -----------------------
		++m_userCount;
		this->updateWindowTitle();
		// -----------------------

		m_protocol.sendHandshake(user);
	} catch(eMUCore::exception_t &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " " << e.what();
		m_logger.out();
		this->disconnect(user);
	} catch(std::exception &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "std exception: " << user << " " << e.what();
		m_logger.out();
		this->disconnect(user);
	} catch(...) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " Unknown exception.";
		m_logger.out();
		this->disconnect(user);
	}
}

void connectServer_t::onContextReceive(eMUCore::socketContext_t &context) {
	connectServerUser_t &user = reinterpret_cast<connectServerUser_t&>(context);

	try {
		size_t parsedDataSize = 0;

		do {
			size_t rawDataSize = eMUCore::packet_t::getRawDataSize(&user.getRecvBuffer().m_data[parsedDataSize]);
			eMUCore::packet_t packet(&user.getRecvBuffer().m_data[parsedDataSize]);

			#ifdef _DEBUG
			m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << user << " Received " << packet << ".";
			m_logger.out();
			#endif

			// -------------------------------
			m_protocol.core(user, packet);
			// -------------------------------

			parsedDataSize += rawDataSize;
		} while(parsedDataSize < user.getRecvBuffer().m_dataSize);
	} catch(eMUCore::exception_t &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " " << e.what();
		m_logger.out();
		this->disconnect(user);
	} catch(std::exception &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "std exception: " << user << " " << e.what();
		m_logger.out();
		this->disconnect(user);
	} catch(...) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: " << user << " Unknown exception.";
		m_logger.out();
		this->disconnect(user);
	}
}

void connectServer_t::onContextClose(eMUCore::socketContext_t &context) {
	connectServerUser_t &user = reinterpret_cast<connectServerUser_t&>(context);

	m_logger.in(eMUCore::logger_t::_MESSAGE_INFO) << user << " Disconnected.";
	m_logger.out();

	// -----------------------
	--m_userCount;
	this->updateWindowTitle();
	// -----------------------
}

void connectServer_t::onDatagramReceive(sockaddr_in& /*inetAddr*/, unsigned char* data, size_t dataSize) {
	try {
		size_t parsedDataSize = 0;

		do {
			size_t rawDataSize = eMUCore::packet_t::getRawDataSize(&data[parsedDataSize]);

			eMUCore::packet_t packet(&data[parsedDataSize]);

			if(packet.getProtocolId() == 0x01) {
				unsigned short gameServerCode = packet.read<unsigned short>(3);
				size_t gameServerLoad = packet.read<unsigned char>(5);
				m_game.serverListUpdate(gameServerCode, gameServerLoad);
			}

			parsedDataSize += rawDataSize;
		} while(parsedDataSize < dataSize);
	} catch(eMUCore::exception_t &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Exception: [UDP] " << e.what();
		m_logger.out();
	} catch(std::exception &e) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "std exception: [UDP] " << e.what();
		m_logger.out();
	} catch(...) {
		m_logger.in(eMUCore::logger_t::_MESSAGE_ERROR) << "Unknown exception: [UDP].";
		m_logger.out();
	}
}

void connectServer_t::send(connectServerUser_t &user, const eMUCore::packet_t &packet) {
	#ifdef _DEBUG
	m_logger.in(eMUCore::logger_t::_MESSAGE_PROTOCOL) << user << " Sending " << packet << ".";
	m_logger.out();
	#endif

	m_iocpEngine.write(user, packet.getData(), packet.getSize());
}

void connectServer_t::fatalHandler(int /*signalId*/) {
	//eMUCore::stackTrace_t::dumpCallTree("log\\stackTrace.txt");
}

DWORD WINAPI connectServer_t::consoleEventHandler(unsigned int /*eventId*/) {
	if(!connectServer_t::m_interrupt) {
		int ret = MessageBox(0, "Do you really want to quit the server?", "eMU::ConnectServer", MB_ICONQUESTION | MB_YESNO);
		if(ret == IDYES) {
			connectServer_t::stop();
		}
	}

    return TRUE;
}

void main(int argsCount, char *argsVect[]) {
	signal(SIGABRT, connectServer_t::fatalHandler);
	signal(SIGFPE, connectServer_t::fatalHandler);
	signal(SIGILL, connectServer_t::fatalHandler);
	signal(SIGSEGV, connectServer_t::fatalHandler);
	signal(SIGTERM, connectServer_t::fatalHandler);

	//system("COLOR 0F");
	std::cout << "		+-----------------------------------------------+" << std::endl;
	std::cout << "		|						|" << std::endl;
	std::cout << "		|		eMU :: ConnectServer		|"  << std::endl;
	std::cout << "		|		 Build: " << __DATE__ << "		|" << std::endl;
	std::cout << "		|						|" << std::endl;
	std::cout << "		+-----------------------------------------------+" << std::endl << std::endl;

	try {
		if(SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(connectServer_t::consoleEventHandler), TRUE) != 0) {
			WSADATA wsaData = {0};
			if(WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
				if(argsCount == 4) {
					connectServer_t *connectServer = new connectServer_t(boost::lexical_cast<size_t>(argsVect[1]),
																			boost::lexical_cast<size_t>(argsVect[2]),
																			boost::lexical_cast<unsigned short>(argsVect[3]));
					connectServer->startup();
					connectServer->worker();
					connectServer->cleanup();
					delete connectServer;
				} else {
					eMUCore::exception_t e;
					e.in() << __FILE__ << ":" << __LINE__ << "[main()] Invalid command line.";
					throw e;
				}
			} else {
				eMUCore::exception_t e;
				e.in() << __FILE__ << ":" << __LINE__ << "[main()] WSAStartup failed with error#" << WSAGetLastError() << ".";
				throw e;
			}
		} else {
			eMUCore::exception_t e;
			e.in() << __FILE__ << ":" << __LINE__ << "[main()] SetConsoleCtrlHandler failed with error#" << GetLastError() << ".";
			throw e;
		}
	} catch(eMUCore::exception_t &e) {
		std::cout << "[Exception] " << e.what() << std::endl;
	} catch(std::exception &e) {
		std::cout << "[std exception] " << e.what() << std::endl;
	}

	system("PAUSE");
}

#pragma warning(default: 4355)