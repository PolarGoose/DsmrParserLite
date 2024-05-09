#include "DsmrParser/DsmrParser.h"
#include <cstdio>
#include <doctest.h>
#include <functional>
#include <vector>
using namespace DsmrParser;

class PacketMock : public IPacket {
  const char* data;
  const size_t size;

public:
  PacketMock(const char* data, size_t size) : data(data), size(size) {}

  StringView Data() const override { return StringView(data, size); }
};

class DsmrParserResultReceiverMock : public IDsmrParserResultReceiver {
  std::function<void(const DsmrDataObject&)> onDsmrData;

public:
  void SetCallback(std::function<void(const DsmrDataObject&)> callback) { onDsmrData = callback; }

  void OnDsmrData(const DsmrDataObject& dsmrData) override { onDsmrData(dsmrData); }
};

TEST_CASE("DsmrPacketParser") {
  DsmrParserResultReceiverMock resultReceiver;
  DsmrPacketParser parser(resultReceiver);

  SUBCASE("Correct header") {
    const char packetData[] = "/Ene5identification identification\r\ndata";
    PacketMock packetMock(packetData, sizeof(packetData));
    DsmrPacketHeader header;
    REQUIRE(parser.ParseHeader(packetMock, header) == true);
    char str[200] = {};
    strncpy(str, header.identification.Data(), header.identification.Size());

    REQUIRE(strncmp(header.version, "Ene5", 4) == 0);
    REQUIRE(header.identification == "identification identification");
  }

  SUBCASE("Incorrect header") {
    const char packetData[] = "Ene5identification identification\r\ndata";
    PacketMock packetMock(packetData, sizeof(packetData));
    DsmrPacketHeader header;
    REQUIRE(parser.ParseHeader(packetMock, header) == false);
  }

  SUBCASE("Parsing data") {
    const char packetData[] = "/Ene5\\XS210 ESMR 5.0\r\n"
                              "\r\n"
                              "1-3:0.2.8(50)\r\n"
                              "0-0:1.0.0(231017090442S)\r\n"
                              "0-0:96.1.1(4530303437303030303434363636353138)\r\n"
                              "1-0:1.8.1(008243.448*kWh)\r\n"
                              "1-0:1.8.2(010196.219*kWh)\r\n"
                              "1-0:2.8.1(000000.005*kWh)\r\n"
                              "1-0:2.8.2(000000.000*kWh)\r\n"
                              "0-0:96.14.0(0002)\r\n"
                              "1-0:1.7.0(03.229*kW)\r\n"
                              "1-0:2.7.0(00.000*kW)\r\n"
                              "0-0:96.7.21(00103)\r\n"
                              "0-0:96.7.9(00004)\r\n"
                              "1-0:99.97.0(3)(0-0:96.7.19)(230608111028S)(0000000500*s)(220127110938W)(0000007650*s)(200331155338S)(0000004144*s)\r\n"
                              "1-0:32.32.0(00009)\r\n"
                              "1-0:32.36.0(00000)\r\n"
                              "0-0:96.13.0()\r\n"
                              "1-0:32.7.0(222.0*V)\r\n"
                              "1-0:31.7.0(014*A)\r\n"
                              "1-0:21.7.0(03.229*kW)\r\n"
                              "1-0:22.7.0(00.000*kW)\r\n"
                              "0-1:24.1.0(003)\r\n"
                              "0-1:96.1.0(4730303539303033393036323731353139)\r\n"
                              "0-1:24.2.1(231017090000S)(04547.595*m3)\r\n"
                              "!E164\r\n";
    PacketMock packetMock(packetData, sizeof(packetData));
    std::vector<DsmrDataObject> dataObjects;
    const auto& callback = [&](const DsmrDataObject& dsmrData) { dataObjects.push_back(dsmrData); };

    resultReceiver.SetCallback([&](const DsmrDataObject& dsmrData) { dataObjects.push_back(dsmrData); });

    parser.Parse(packetMock);

    REQUIRE(dataObjects.size() == 19);
    REQUIRE(dataObjects[0].obisCode.A == 1);
    REQUIRE(dataObjects[0].obisCode.B == 3);
    REQUIRE(dataObjects[0].obisCode.C == 0);
    REQUIRE(dataObjects[0].obisCode.D == 2);
    REQUIRE(dataObjects[0].obisCode.E == 8);
    REQUIRE(dataObjects[0].value == "50");
    REQUIRE(dataObjects[0].unit.Data() == nullptr);
    REQUIRE(dataObjects[2].value == "008243.448");
    REQUIRE(dataObjects[2].unit == "kWh");
  }
}
