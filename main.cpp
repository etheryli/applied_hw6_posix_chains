/*
 * Hung Nguyen
 * PID: dnhung7@vt.edu
 * ID#: 905925175
 * Course: ECE3574
 * Assignment: Homework 6
 * File:
 * Honor Code:
*/
#include <QByteArray>
#include <QList>
#include <QString>
#include <QTextStream>
#include <errno.h>
#include <mqueue.h>
#include <pthread.h>
#include <string>
#define MSG_LEN 8192
#define MODE 0664

QTextStream cerr(stderr);
QTextStream cout(stdout);

void process0(double &m_temp) {
  mqd_t mailbox70, mailbox71, mailbox72;

  // Create read-write, exclusive queue with 0644 (permission) and default size
  mailbox70 = mq_open("/70", O_RDWR | O_CREAT | O_EXCL, MODE, NULL);

  // Error for duplicate processes
  if (mailbox70 == -1) {
    cerr << "Error: Trying to run a process twice\n";
    exit(0);
  }

  // Variable init
  double current_temp = m_temp;
  double up_temp1, up_temp2;

  // Buffers to send and receive
  QString sendString1, sendString2;
  QByteArray sendByteArray1, sendByteArray2;
  char *receiveBuffer1 = new char[MSG_LEN];
  char *receiveBuffer2 = new char[MSG_LEN];
  char *sendBuffer1 = new char();
  char *sendBuffer2 = new char();
  QString message1, message2;

  mailbox71 = mq_open("/71", O_RDWR, NULL, NULL);
  mailbox72 = mq_open("/72", O_RDWR, NULL, NULL);

  // Convert to message pointer to send
  sendString1 = "start";
  sendString2 = sendString1;
  sendByteArray1 = sendString1.toLocal8Bit();
  sendByteArray2 = sendString2.toLocal8Bit();
  sendBuffer1 = sendByteArray1.data();
  sendBuffer2 = sendByteArray2.data();

  // Down send the message pointers with priority 1
  mq_send(mailbox71, sendBuffer1, MSG_LEN, 1);
  mq_send(mailbox72, sendBuffer2, MSG_LEN, 1);

  // Loop until stabilized
  while (1) {
    // Get the message
    mq_receive(mailbox71, receiveBuffer1, MSG_LEN, NULL);
    mq_receive(mailbox72, receiveBuffer2, MSG_LEN, NULL);
    message1 = QString::fromLocal8Bit(receiveBuffer1);
    message2 = QString::fromLocal8Bit(receiveBuffer2);

    // Convert the up message to double
    up_temp1 = message1.toDouble();
    up_temp2 = message2.toDouble();

    // Average
    current_temp = (up_temp1 + up_temp2 + current_temp) / 2.0;

    // Terminate if less than 1/100 decimal, and also send to children
    if (qAbs(current_temp - m_temp) < 0.01) {
      m_temp = current_temp;

      // Convert to message pointer to send
      sendString1 = "stable";
      sendString2 = sendString1;
      sendByteArray1 = sendString1.toLocal8Bit();
      sendByteArray2 = sendString2.toLocal8Bit();
      sendBuffer1 = sendByteArray1.data();
      sendBuffer2 = sendByteArray2.data();

      // Down send the message pointers with priority 1
      mq_send(mailbox71, sendBuffer1, MSG_LEN, 1);
      mq_send(mailbox72, sendBuffer2, MSG_LEN, 1);
      break;
    }

    // Convert to message pointer to send
    sendString1 = QString::number(current_temp);
    sendString2 = sendString1;
    sendByteArray1 = sendString1.toLocal8Bit();
    sendByteArray2 = sendString2.toLocal8Bit();
    sendBuffer1 = sendByteArray1.data();
    sendBuffer2 = sendByteArray2.data();

    // Down send the message pointers with priority 1
    mq_send(mailbox71, sendBuffer1, MSG_LEN, 1);
    mq_send(mailbox72, sendBuffer2, MSG_LEN, 1);

    // Print to console
    cout << "Process #0: current temperature " << current_temp << "\n";
  }

  mq_unlink("/70");
  mq_close(mailbox70);
}

void process1(double &m_temp) {
  mqd_t mailbox71, mailbox70, mailbox73, mailbox74;

  // Create read-write, exclusive queue with 0644 (permission) and default size
  mailbox71 = mq_open("/71", O_RDWR | O_CREAT | O_EXCL, MODE, NULL);

  // Error for duplicate processes
  if (mailbox71 == -1) {
    cerr << "Error: Trying to run a process twice\n";
    exit(0);
  }

  // Direction to receive/send
  // (false is down, true is up)
  bool direction = false;

  // Variable init
  double current_temp = m_temp;
  double down_temp, up_temp1, up_temp2;

  // Buffers to send and receive
  QString sendString1, sendString2;
  QByteArray sendByteArray1, sendByteArray2;
  char *receiveBuffer1 = new char[MSG_LEN];
  char *receiveBuffer2 = new char[MSG_LEN];
  char *sendBuffer1 = new char();
  char *sendBuffer2 = new char();
  QString message1, message2;

  // Loop until stabilized
  while (1) {
    // Get the message
    mq_receive(mailbox71, receiveBuffer1, MSG_LEN, NULL);
    message1 = QString::fromLocal8Bit(receiveBuffer1);

    // Wait on start message to connect to parent and children
    // Skips everything else after starting
    if (message1 == "start") {
      mailbox70 = mq_open("/70", O_RDWR, NULL, NULL);
      mailbox73 = mq_open("/73", O_RDWR, NULL, NULL);
      mailbox74 = mq_open("/74", O_RDWR, NULL, NULL);

      // Send status message to children
      // Convert to message pointer to send
      sendString1 = "start";
      sendString2 = sendString1;
      sendByteArray1 = sendString1.toLocal8Bit();
      sendByteArray2 = sendString2.toLocal8Bit();
      sendBuffer1 = sendByteArray1.data();
      sendBuffer2 = sendByteArray2.data();

      // Send the message pointer (up pass) with priority 1
      mq_send(mailbox73, sendBuffer1, MSG_LEN, 1);
      mq_send(mailbox74, sendBuffer2, MSG_LEN, 1);
      continue;
    }

    // Terminate if stable, also send stable to children
    if (message1 == "stable") {
      m_temp = current_temp;

      // Convert to message pointer to send
      sendString1 = "stable";
      sendString2 = sendString1;
      sendByteArray1 = sendString1.toLocal8Bit();
      sendByteArray2 = sendString2.toLocal8Bit();
      sendBuffer1 = sendByteArray1.data();
      sendBuffer2 = sendByteArray2.data();

      // Send the message pointer (up pass) with priority 1
      mq_send(mailbox73, sendBuffer1, MSG_LEN, 1);
      mq_send(mailbox74, sendBuffer2, MSG_LEN, 1);
      // Send stable message to children
      break;
    }

    // if Direction is up (true) receive another and send twice
    if (direction) {
      // Retrieve the other message
      mq_receive(mailbox71, receiveBuffer2, MSG_LEN, NULL);
      message1 = QString::fromLocal8Bit(receiveBuffer2);

      // Convert the up messages to doubles
      up_temp1 = message1.toDouble();
      up_temp2 = message2.toDouble();

      // Average
      current_temp = (up_temp1 + up_temp2 + current_temp) / 3.0;

      // Send to parent mailbox0 with current temp and priority 1
      sendString1 = QString::number(current_temp);
      sendByteArray1 = sendString1.toLocal8Bit();
      sendBuffer1 = sendByteArray1.data();
      mq_send(mailbox70, sendBuffer1, MSG_LEN, 1);

      // Change to down direction (false)
      direction = false;

      // Print to console
      cout << "Process #1: current temperature " << current_temp << "\n";
      continue;
    }
    // Else for down direction
    // Convert the down message to double
    down_temp = message1.toDouble();

    // Average
    current_temp = (down_temp + current_temp) / 2.0;

    // Convert to message pointer to send
    sendString1 = QString::number(current_temp);
    sendString2 = sendString1;
    sendByteArray1 = sendString1.toLocal8Bit();
    sendByteArray2 = sendString2.toLocal8Bit();
    sendBuffer1 = sendByteArray1.data();
    sendBuffer2 = sendByteArray2.data();

    // Send the message pointer (up pass) with priority 1
    mq_send(mailbox73, sendBuffer1, MSG_LEN, 1);
    mq_send(mailbox74, sendBuffer2, MSG_LEN, 1);

    direction = true;

    // Print to console
    cout << "Process #1: current temperature " << current_temp << "\n";
  }

  mq_unlink("/71");
  mq_close(mailbox71);
}

void process2(double &m_temp) {
  mqd_t mailbox72, mailbox70, mailbox75, mailbox76;

  // Create read-write, exclusive queue with 0644 (permission) and default size
  mailbox72 = mq_open("/72", O_RDWR | O_CREAT | O_EXCL, MODE, NULL);

  // Error for duplicate processes
  if (mailbox72 == -1) {
    cerr << "Error: Trying to run a process twice\n";
    exit(0);
  }

  // Direction to receive/send
  // (false is down, true is up)
  bool direction = false;

  // Variable init
  double current_temp = m_temp;
  double down_temp, up_temp1, up_temp2;

  // Buffers to send and receive
  QString sendString1, sendString2;
  QByteArray sendByteArray1, sendByteArray2;
  char *receiveBuffer1 = new char[MSG_LEN];
  char *receiveBuffer2 = new char[MSG_LEN];
  char *sendBuffer1 = new char();
  char *sendBuffer2 = new char();
  QString message1, message2;

  // Loop until stabilized
  while (1) {
    // Get the message
    mq_receive(mailbox72, receiveBuffer1, MSG_LEN, NULL);
    message1 = QString::fromLocal8Bit(receiveBuffer1);

    // Wait on start message to connect to parent and children
    // Skips everything else after starting
    if (message1 == "start") {
      mailbox70 = mq_open("/70", O_RDWR, NULL, NULL);
      mailbox75 = mq_open("/75", O_RDWR, NULL, NULL);
      mailbox76 = mq_open("/76", O_RDWR, NULL, NULL);

      // Convert to message pointer to send
      sendString1 = "start";
      sendString2 = sendString1;
      sendByteArray1 = sendString1.toLocal8Bit();
      sendByteArray2 = sendString2.toLocal8Bit();
      sendBuffer1 = sendByteArray1.data();
      sendBuffer2 = sendByteArray2.data();

      // Send the message pointer (up pass) with priority 1
      mq_send(mailbox75, sendBuffer1, MSG_LEN, 1);
      mq_send(mailbox76, sendBuffer2, MSG_LEN, 1);
      continue;
    }

    // Terminate if stable, also send stable to children
    if (message1 == "stable") {
      m_temp = current_temp;

      // Convert to message pointer to send
      sendString1 = "stable";
      sendString2 = sendString1;
      sendByteArray1 = sendString1.toLocal8Bit();
      sendByteArray2 = sendString2.toLocal8Bit();
      sendBuffer1 = sendByteArray1.data();
      sendBuffer2 = sendByteArray2.data();

      // Send the message pointer (up pass) with priority 1
      mq_send(mailbox75, sendBuffer1, MSG_LEN, 1);
      mq_send(mailbox76, sendBuffer2, MSG_LEN, 1);
      break;
    }

    // if Direction is up (true) receive another and send twice
    if (direction) {
      // Retrieve the other message
      mq_receive(mailbox72, receiveBuffer2, MSG_LEN, NULL);
      message1 = QString::fromLocal8Bit(receiveBuffer2);

      // Convert the up messages to doubles
      up_temp1 = message1.toDouble();
      up_temp2 = message2.toDouble();

      // Average
      current_temp = (up_temp1 + up_temp2 + current_temp) / 3.0;

      // Send to parent mailbox0 with current temp and priority 1
      sendString1 = QString::number(current_temp);
      sendByteArray1 = sendString1.toLocal8Bit();
      sendBuffer1 = sendByteArray1.data();
      mq_send(mailbox70, sendBuffer1, MSG_LEN, 1);

      // Change to down direction (false)
      direction = false;

      // Print to console
      cout << "Process #2: current temperature " << current_temp << "\n";
      continue;
    }
    // Else for down direction
    // Convert the down message to double
    down_temp = message1.toDouble();

    // Average
    current_temp = (down_temp + current_temp) / 2.0;

    // Convert to message pointer to send
    sendString1 = QString::number(current_temp);
    sendString2 = sendString1;
    sendByteArray1 = sendString1.toLocal8Bit();
    sendByteArray2 = sendString2.toLocal8Bit();
    sendBuffer1 = sendByteArray1.data();
    sendBuffer2 = sendByteArray2.data();

    // Send the message pointer (up pass) with priority 1
    mq_send(mailbox75, sendBuffer1, MSG_LEN, 1);
    mq_send(mailbox76, sendBuffer2, MSG_LEN, 1);

    direction = true;

    // Print to console
    cout << "Process #2: current temperature " << current_temp << "\n";
  }

  mq_unlink("/72");
  mq_close(mailbox72);
}

void process3(double &m_temp) {
  mqd_t mailbox73, mailbox71;

  // Create read-write, exclusive queue with 0644 (permission) and default size
  mailbox73 = mq_open("/73", O_RDWR | O_CREAT | O_EXCL, MODE, NULL);

  // Error for duplicate processes
  if (mailbox73 == -1) {
    cerr << "Error: Trying to run a process twice\n";
    exit(0);
  }

  // Variable init
  double current_temp = m_temp;
  double down_temp;

  // Buffers to send and receive
  QString sendString;
  QByteArray sendByteArray;
  char *receiveBuffer = new char[MSG_LEN];
  char *sendBuffer = new char();
  QString message;

  // Loop until stabilized
  while (1) {
    // Get the message (down pass)
    mq_receive(mailbox73, receiveBuffer, MSG_LEN, NULL);
    message = QString::fromLocal8Bit(receiveBuffer);

    // Wait on start message to connect to parent
    // Skips everything else after starting
    if (message == "start") {
      mailbox71 = mq_open("/71", O_RDWR, NULL, NULL);
      continue;
    }

    // Terminate if stable
    if (message == "stable") {
      m_temp = current_temp;
      break;
    }

    // Convert the down message to double
    down_temp = message.toDouble();

    // Average
    current_temp = (down_temp + current_temp) / 2.0;

    // Convert to message pointer to send
    sendString = QString::number(current_temp);
    sendByteArray = sendString.toLocal8Bit();
    sendBuffer = sendByteArray.data();

    // Send the message pointer (up pass) with priority 1
    mq_send(mailbox71, sendBuffer, MSG_LEN, 1);

    // Print to console
    cout << "Process #3: current temperature " << current_temp << "\n";
  }

  mq_unlink("/73");
  mq_close(mailbox73);
}

void process4(double &m_temp) {
  mqd_t mailbox74, mailbox71;

  // Create read-write, exclusive queue with 0644 (permission) and default size
  mailbox74 = mq_open("/74", O_RDWR | O_CREAT | O_EXCL, MODE, NULL);

  // Error for duplicate processes
  if (mailbox74 == -1) {

    cerr << "Error: Trying to run a process twice\n";
    exit(0);
  }

  // Variable init
  double current_temp = m_temp;
  double down_temp;

  // Buffers to send and receive
  QString sendString;
  QByteArray sendByteArray;
  char *receiveBuffer = new char[MSG_LEN];
  char *sendBuffer = new char();
  QString message;

  // Loop until stabilized
  while (1) {
    // Get the message (down pass)
    mq_receive(mailbox74, receiveBuffer, MSG_LEN, NULL);
    message = QString::fromLocal8Bit(receiveBuffer);

    // Wait on start message to connect to parent
    // Skips everything else after starting
    if (message == "start") {
      mailbox71 = mq_open("/71", O_RDWR, NULL, NULL);
      continue;
    }

    // Terminate if stable
    if (message == "stable") {
      m_temp = current_temp;
      break;
    }

    // Convert the down message to double
    down_temp = message.toDouble();

    // Average
    current_temp = (down_temp + current_temp) / 2.0;

    // Convert to message pointer to send
    sendString = QString::number(current_temp);
    sendByteArray = sendString.toLocal8Bit();
    sendBuffer = sendByteArray.data();

    // Send the message pointer (up pass) with priority 1
    mq_send(mailbox71, sendBuffer, MSG_LEN, 1);

    // Print to console
    cout << "Process #4: current temperature " << current_temp << "\n";
  }

  mq_unlink("/74");
  mq_close(mailbox74);
}

void process5(double &m_temp) {

  mqd_t mailbox75, mailbox72;

  // Create read-write, exclusive queue with 0644 (permission) and default size
  mailbox75 = mq_open("/75", O_RDWR | O_CREAT | O_EXCL, MODE, NULL);

  // Error for duplicate processes
  if (mailbox75 == -1) {
    cerr << "Error: Trying to run a process twice\n";
    exit(0);
  }

  // Variable init
  double current_temp = m_temp;
  double down_temp;

  // Buffers to send and receive
  QString sendString;
  QByteArray sendByteArray;
  char *receiveBuffer = new char[MSG_LEN];
  char *sendBuffer = new char();
  QString message;

  // Loop until stabilized
  while (1) {
    // Get the message (down pass)
    mq_receive(mailbox75, receiveBuffer, MSG_LEN, NULL);
    message = QString::fromLocal8Bit(receiveBuffer);

    // Wait on start message to connect to parent
    // Skips everything else after starting
    if (message == "start") {
      mailbox72 = mq_open("/72", O_RDWR, NULL, NULL);
      continue;
    }

    // Terminate if stable
    if (message == "stable") {
      m_temp = current_temp;
      break;
    }

    // Convert the down message to double
    down_temp = message.toDouble();

    // Average
    current_temp = (down_temp + current_temp) / 2.0;

    // Convert to message pointer to send
    sendString = QString::number(current_temp);
    sendByteArray = sendString.toLocal8Bit();
    sendBuffer = sendByteArray.data();

    // Send the message pointer (up pass) with priority 1
    mq_send(mailbox72, sendBuffer, MSG_LEN, 1);

    // Print to console
    cout << "Process #5: current temperature " << current_temp << "\n";
  }

  mq_unlink("/75");
  mq_close(mailbox75);
}

void process6(double &m_temp) {

  mqd_t mailbox76, mailbox72;

  // Create read-write, exclusive queue with 0644 (permission) and default size
  mailbox76 = mq_open("/76", O_RDWR | O_CREAT | O_EXCL, MODE, NULL);

  // Error for duplicate processes
  if (mailbox76 == -1) {
    cerr << "Error: Trying to run a process twice\n";
    exit(0);
  }

  // Variable init
  double current_temp = m_temp;
  double down_temp;

  // Buffers to send and receive
  QString sendString;
  QByteArray sendByteArray;
  char *receiveBuffer = new char[MSG_LEN];
  char *sendBuffer = new char();
  QString message;

  // Loop until stabilized
  while (1) {
    // Get the message (down pass)
    mq_receive(mailbox76, receiveBuffer, MSG_LEN, NULL);
    message = QString::fromLocal8Bit(receiveBuffer);

    // Wait on start message to connect to parent
    // Skips everything else after starting
    if (message == "start") {
      mailbox72 = mq_open("/72", O_RDWR, NULL, NULL);
      continue;
    }

    // Terminate if stable
    if (message == "stable") {
      m_temp = current_temp;
      break;
    }

    // Convert the down message to double
    down_temp = message.toDouble();

    // Average
    current_temp = (down_temp + current_temp) / 2.0;

    // Convert to message pointer to send
    sendString = QString::number(current_temp);
    sendByteArray = sendString.toLocal8Bit();
    sendBuffer = sendByteArray.data();

    // Send the message pointer (up pass) with priority 1
    mq_send(mailbox72, sendBuffer, MSG_LEN, 1);

    // Print to console
    cout << "Process #6: current temperature " << current_temp << "\n";
  }

  mq_unlink("/76");
  mq_close(mailbox76);
}

int main(int argc, char *argv[]) {

  // Parse in arguments
  // Error checking for cmdline arguments
  if (argc != 3) {
    cerr << "Usage: ./external <id> <initial_temperature>\n";
    return 1;
  }

  // Error checking for valid id
  int m_id;
  bool id_converted;
  m_id = QString(argv[1]).toInt(&id_converted);

  if (!id_converted) {
    cerr << "Incorrect ID entered. Please enter ID# between 0 and 6.\n";
    return 1;
  }

  if (m_id < 0 || m_id > 6) {
    cerr << "Incorrect ID entered. Please enter ID# between 0 and 6.\n";
    return 1;
  }

  // Error checking for valid temperature
  double m_temp;
  bool temp_converted;
  m_temp = QString(argv[2]).toDouble(&temp_converted);
  if (!temp_converted) {
    cerr << "Unknown temperature entered. Please enter a valid decimal "
            "temperature.\n";
    return 1;
  }

  // Process base on id number
  switch (m_id) {
  case 0:
    process0(m_temp);
    break;
  case 1:
    process1(m_temp);
    break;
  case 2:
    process2(m_temp);
    break;
  case 3:
    process3(m_temp);
    break;
  case 4:
    process4(m_temp);
    break;
  case 5:
    process5(m_temp);
    break;
  case 6:
    process6(m_temp);
    break;
  default:
    cerr << "How did you get here...\n";
    break;
  }
  // prints final temperature
  cout << " Process #" << m_id << ": final temperature " << m_temp << "\n";

  return 0;
}
