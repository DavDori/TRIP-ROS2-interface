#include "serial_interface.h"

SerialInterface::SerialInterface(const std::string& port, speed_t baud_rate)
{
    connect(port, baud_rate);
    last_message_ = "";
}

SerialInterface::SerialInterface()
{
    serial_handle_ = INVALID_HANDLE;
    baud_rate_ =  0;
    last_message_ = "";
}

void SerialInterface::connect(const std::string& port, speed_t baud_rate) {
    disconnect(); // close old connection if present
    serial_handle_ = open( port.c_str(), O_RDWR| O_NOCTTY );
    baud_rate_ = baud_rate;
    if ( INVALID_HANDLE == serial_handle_ )
    {
        throw std::runtime_error("Error: cannot open Serial Port!");
    }

    initSerialPort();
}

void SerialInterface::initSerialPort()
{
    if (!isConnected())
        throw std::runtime_error("Error: Serial port is not open!");
    struct termios tty;
    tcgetattr(serial_handle_, &tty);

      // Read in existing settings, and handle any error
    if(tcgetattr(serial_handle_, &tty) != 0) {
        throw std::runtime_error("Error: cannot find any connected device!");
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    tty.c_cc[VTIME] = 0;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;
    // Set the Tx and Rx Baud Rate to BOUDRATE
    cfsetospeed(&tty, (speed_t)baud_rate_);
    cfsetispeed(&tty, (speed_t)baud_rate_);


    /* Flush the Input buffer and set the attribute NOW without waiting for Data to Complete*/
    //flushReceiver();
    ioctl(serial_handle_, TCFLSH, 2); // flush both
    // write port configuration to driver
    tcsetattr(serial_handle_, TCSANOW, &tty);

}

void SerialInterface::disconnect() {
    if (isConnected())
        close(serial_handle_);

    serial_handle_ = INVALID_HANDLE;
}

bool SerialInterface::isConnected() const {
    return serial_handle_ != INVALID_HANDLE;
}


void SerialInterface::send(const std::string& message)
{
    if (!isConnected())
        throw std::runtime_error("Error: trying to send message but serial port is not open.");


    int count_sent = write(serial_handle_, message.c_str(), message.length());

    // Verify weather the Transmitting Data on UART was Successful or Not
    if (count_sent < 0)
        throw std::runtime_error("Error: trying to send message but transmission failed.");
}

std::string SerialInterface::readLine()
{
    if(!isConnected())
        throw std::runtime_error("Error: trying to read message but serial port is not open.");
    char c;
    std::string message = "";
    int bytes_read;
    while ((bytes_read = read(serial_handle_, &c, 1)) > 0) 
    {
        if (bytes_read > 0) {
            if (c == '\n' || c == '\r') {
                break;
            }
            message.push_back(c);
        } else if (bytes_read < 0) {
            break;
        }
    }
    if (bytes_read < 0) {
        if (errno == EAGAIN)
            throw std::runtime_error("Warning: serial io.");
        else
            throw std::runtime_error("Warning: trying to send message but transmission failed.");
    }
    if(message.size() != 0)
        last_message_ = message;
    ioctl(serial_handle_, TCFLSH, 0); // flush this
    return message;
}

void SerialInterface::flushReceiver()
{
    tcflush(serial_handle_, TCIFLUSH);
}
