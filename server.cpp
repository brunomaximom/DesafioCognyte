#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <ctime>
#include <boost/bind/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
using boost::asio::ip::tcp;

const int max_length = 1024;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

void session(socket_ptr sock, int max_filesize)
{
  try
  {
    int counter = 0;
    for (;;)
    {
      char data[max_length], tmp[max_filesize];
      boost::system::error_code error;
      size_t length = sock->read_some(boost::asio::buffer(data), error);

      while (counter <= length)
      {
        /* Create files with received data */
        char filename[255];
        struct tm* tm;
        time_t now;
        now = time(0);
        tm = localtime(&now);
        sprintf(filename, "output_%04d%02d%02d%02d%02d%02d.txt", 
            tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        ofstream OutputFile(filename);
        
        for (int i = 0; i < max_filesize; i++) tmp[i] = data[i+counter];
        OutputFile << tmp;
        OutputFile.close();
        counter += max_filesize;
        sleep(1);
      }

      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      boost::asio::write(*sock, boost::asio::buffer(data, length));
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}

void server(boost::asio::io_context& io_context, unsigned short port, int max_filesize)
{
  tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
  for (;;)
  {
    socket_ptr sock(new tcp::socket(io_context));
    a.accept(*sock);
    boost::thread t(boost::bind(session, sock, max_filesize));
  }
}

int main(int argc, char* argv[])
{
  try
  {
    int port, max_filesize;
    string line;
    vector<string> strs;
  
    /* Read config file */
    ifstream ConfigFile("config.cfg");
    getline (ConfigFile, line);
    boost::split(strs,line,boost::is_any_of(" "));
    port = stoi(strs[1]);
    getline (ConfigFile, line);
    boost::split(strs,line,boost::is_any_of(" "));
    max_filesize = stoi(strs[1]);

    boost::asio::io_context io_context;

    using namespace std; 
    server(io_context, port, max_filesize);
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}