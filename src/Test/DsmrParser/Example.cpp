#include "DsmrParser/DsmrParser.h"
#include <cstdio>
#include <doctest.h>

class DsmrParserResultReceiver : public DsmrParser::IDsmrParserResultReceiver {
  void OnDsmrData(const DsmrParser::DsmrDataObject& dsmrData) override {
    // Print the parsed DSMR data object
    printf("obisCode: %d-%d:%d.%d.%d", dsmrData.obisCode.A, dsmrData.obisCode.B, dsmrData.obisCode.C, dsmrData.obisCode.D, dsmrData.obisCode.E);
    printf(" value: '%.*s'", (unsigned int)dsmrData.value.Size(), dsmrData.value.Data());
    printf(" unit: '%.*s'\n", (unsigned int)dsmrData.unit.Size(), dsmrData.unit.Data());
  }
};

const char dsmrData[] = "/Ene5\\XS210 ESMR 5.0\r\n"
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
                        "!E164\r\n"

                        "/Ene5\\XS210 ESMR 5.0\r\n"
                        "\r\n"
                        "1-3:0.2.8(50)\r\n"
                        "0-0:1.0.0(231017090443S)\r\n"
                        "0-0:96.1.1(4530303437303030303434363636353138)\r\n"
                        "1-0:1.8.1(008243.448*kWh)\r\n"
                        "1-0:1.8.2(010196.220*kWh)\r\n"
                        "1-0:2.8.1(000000.005*kWh)\r\n"
                        "1-0:2.8.2(000000.000*kWh)\r\n"
                        "0-0:96.14.0(0002)\r\n"
                        "1-0:1.7.0(03.218*kW)\r\n"
                        "1-0:2.7.0(00.000*kW)\r\n"
                        "0-0:96.7.21(00103)\r\n"
                        "0-0:96.7.9(00004)\r\n"
                        "1-0:99.97.0(3)(0-0:96.7.19)(230608111028S)(0000000500*s)(220127110938W)(0000007650*s)(200331155338S)(0000004144*s)\r\n"
                        "1-0:32.32.0(00009)\r\n"
                        "1-0:32.36.0(00000)\r\n"
                        "0-0:96.13.0()\r\n"
                        "1-0:32.7.0(222.0*V)\r\n"
                        "1-0:31.7.0(014*A)\r\n"
                        "1-0:21.7.0(03.218*kW)\r\n"
                        "1-0:22.7.0(00.000*kW)\r\n"
                        "0-1:24.1.0(003)\r\n"
                        "0-1:96.1.0(4730303539303033393036323731353139)\r\n"
                        "0-1:24.2.1(231017090000S)(04547.595*m3)\r\n"
                        "!030C\r\n";

TEST_CASE("Example of how to use DsmrParser library") {
  DsmrParserResultReceiver resultReceiver;
  DsmrParser::DsmrPacketReceiver<4000> receiver;
  DsmrParser::DsmrPacketParser parser(resultReceiver);

  // Simulate reading of DSMR data from serial port one byte at a time
  for (const auto& byte : dsmrData) {

    // Feed the byte to the DSMR packet receiver until a full packet is received
    const DsmrParser::IPacket* packet = receiver.ProcessByte(byte);

    if (packet != nullptr) {
      printf("Packet received");

      // When packet is received we can parse it
      DsmrParser::DsmrPacketHeader header;

      // Parse the header (this step is optional)
      if (parser.ParseHeader(*packet, header) == true) {
        printf("Parsed header:\n");
        printf("version: '%.*s'\n", (unsigned int)sizeof(header.version), header.version);
        printf("Identification: '%.*s'\n", (unsigned int)header.identification.Size(), header.identification.Data());
      }

      // Parse the DSMR data objects. This method will call DsmrParserResultReceiver::OnDsmrData for each parsed object
      parser.Parse(*packet);
    }
  }
}
