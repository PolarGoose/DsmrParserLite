#include "DsmrParser/DsmrParser.h"
#include <doctest.h>
using namespace DsmrParser;

TEST_CASE("DsmrPacketReceiver") {
  DsmrPacketReceiver<4000> receiver;

  SUBCASE("Receive correct packet") {
    const char packetData[] = "/some data"
                              "data"
                              "!02AD";
    bool packetReceived = false;

    for (const auto& byte : packetData) {
      const auto& packet = receiver.ProcessByte(byte);
      if (packet != nullptr) {
        packetReceived = true;
        REQUIRE(packet->Data().Size() == 15);
      }
    }
    REQUIRE(packetReceived);
  }

  SUBCASE("Packet with incorrect CRC16") {
    const char packetData[] = "/some data"
                              "data"
                              "!AAAA";
    for (const auto& byte : packetData) {
      REQUIRE(receiver.ProcessByte(byte) == nullptr);
    }
  }

  SUBCASE("Packet with the beginning symbol in the middle") {
    const char packetData[] = "/some data"
                              "da/ta"
                              "!AAAA";
    for (const auto& byte : packetData) {
      REQUIRE(receiver.ProcessByte(byte) == nullptr);
    }
  }

  SUBCASE("Packet with the beginning symbol in the middle") {
    const char packetData[] = "/some data"
                              "da/ta"
                              "!AAAA";
    for (const auto& byte : packetData) {
      REQUIRE(receiver.ProcessByte(byte) == nullptr);
    }
  }

  SUBCASE("Packet with the garbage in the beginning") {
    const char packetData[] = "garbage"
                              "/some data"
                              "data"
                              "!02AD";
    bool packetReceived = false;

    for (const auto& byte : packetData) {
      if (receiver.ProcessByte(byte) != nullptr) {
        packetReceived = true;
      }
    }
    REQUIRE(packetReceived);
  }

  SUBCASE("Packet with wrong CRC symbols") {
    const char packetData[] = "garbage"
                              "/some data"
                              "data"
                              "!0T12";
    for (const auto& byte : packetData) {
      REQUIRE(receiver.ProcessByte(byte) == nullptr);
    }
  }

  SUBCASE("BufferOverflow") {
    const char packetData[] = "garbage"
                              "/some data"
                              "data"
                              "!0T12";
    for (const auto& byte : packetData) {
      REQUIRE(receiver.ProcessByte(byte) == nullptr);
    }
  }

  SUBCASE("Receive several packets") {
    const char packetData[] = "/some data"
                              "data"
                              "!02AD"
                              "/some data"
                              "data"
                              "!AAAA"
                              "/some data"
                              "data"
                              "!02AD";
    size_t packetReceived = 0;

    for (const auto& byte : packetData) {
      const auto& packet = receiver.ProcessByte(byte);
      if (packet != nullptr) {
        packetReceived++;
      }
    }
    REQUIRE(packetReceived == 2);
  }

  SUBCASE("CRC calculation test") {
    const char packetData[] = "/KFM5KAIFA-METER\r\n"
                              "\r\n"
                              "1-3:0.2.8(40)\r\n"
                              "0-0:1.0.0(150117185916W)\r\n"
                              "0-0:96.1.1(0000000000000000000000000000000000)\r\n"
                              "1-0:1.8.1(000671.578*kWh)\r\n"
                              "1-0:1.8.2(000842.472*kWh)\r\n"
                              "1-0:2.8.1(000000.000*kWh)\r\n"
                              "1-0:2.8.2(000000.000*kWh)\r\n"
                              "0-0:96.14.0(0001)\r\n"
                              "1-0:1.7.0(00.333*kW)\r\n"
                              "1-0:2.7.0(00.000*kW)\r\n"
                              "0-0:17.0.0(999.9*kW)\r\n"
                              "0-0:96.3.10(1)\r\n"
                              "0-0:96.7.21(00008)\r\n"
                              "0-0:96.7.9(00007)\r\n"
                              "1-0:99.97.0(1)(0-0:96.7.19)(000101000001W)(2147483647*s)\r\n"
                              "1-0:32.32.0(00000)\r\n"
                              "1-0:32.36.0(00000)\r\n"
                              "0-0:96.13.1()\r\n"
                              "0-0:96.13.0()\r\n"
                              "1-0:31.7.0(001*A)\r\n"
                              "1-0:21.7.0(00.332*kW)\r\n"
                              "1-0:22.7.0(00.000*kW)\r\n"
                              "0-1:24.1.0(003)\r\n"
                              "0-1:96.1.0(0000000000000000000000000000000000)\r\n"
                              "0-1:24.2.1(150117180000W)(00473.789*m3)\r\n"
                              "0-1:24.4.0(1)\r\n"
                              "!6F4A\r\n";
    bool packetReceived = false;

    for (const auto& byte : packetData) {
      const auto& packet = receiver.ProcessByte(byte);
      if (packet != nullptr) {
        packetReceived = true;
        REQUIRE(packet->Data().Size() == 672);
      }
    }
    REQUIRE(packetReceived);
  }
}

TEST_CASE("DsmrPacketReceiver BufferOverflow") {
  DsmrPacketReceiver<20> receiver;

  SUBCASE("Receive correct packet") {
    const char packetData[] = "/some data"
                              "datadatadatadatadatadata"
                              "!02AD"
                              "/some data"
                              "data"
                              "!02AD";
    bool packetReceived = false;

    for (const auto& byte : packetData) {
      const auto& packet = receiver.ProcessByte(byte);
      if (packet != nullptr) {
        packetReceived = true;
        REQUIRE(packet->Data().Size() == 15);
      }
    }
    REQUIRE(packetReceived);
  }
}
