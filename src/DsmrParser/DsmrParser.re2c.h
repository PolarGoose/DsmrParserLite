#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace DsmrParser {

class NonCopyableAndNonMovable {
protected:
  NonCopyableAndNonMovable() = default;
  ~NonCopyableAndNonMovable() = default;

public:
  NonCopyableAndNonMovable(NonCopyableAndNonMovable&&) = delete;
  NonCopyableAndNonMovable& operator=(NonCopyableAndNonMovable&&) = delete;

  NonCopyableAndNonMovable(const NonCopyableAndNonMovable&) = delete;
  NonCopyableAndNonMovable& operator=(const NonCopyableAndNonMovable&) = delete;
};

enum class StateId { WaitingForPacketStartSymbol, WaitingForPacketEndSymbol, WaitingForCrc };

class StringView {
  const char* data;
  size_t size;

public:
  StringView() : data(nullptr), size(0) {}
  StringView(const char* data, size_t size) : data(data), size(size) {}

  [[nodiscard]] const char* Data() const { return data; }
  [[nodiscard]] size_t Size() const { return size; }

  friend bool operator==(const StringView& lhs, const StringView& rhs) {
    if (lhs.size != rhs.size) {
      return false;
    }
    return strncmp(lhs.data, rhs.data, lhs.size) == 0;
  }

  friend bool operator==(const StringView& lhs, const char* rhs) { return strncmp(lhs.data, rhs, lhs.size) == 0; }

  friend bool operator!=(const StringView& lhs, const StringView& rhs) { return !(lhs == rhs); }

  friend bool operator!=(const StringView& lhs, const char* rhs) { return !(lhs == rhs); }
};

struct IPacket {
  virtual [[nodiscard]] StringView Data() const = 0;
};

struct IState {
  virtual const IPacket* ProcessByte(char byte) = 0;
};

struct IStateContext {
  virtual void SetState(StateId stateId) = 0;
};

class IPacketBuffer : public IPacket {
public:
  virtual [[nodiscard]] void Add(char byte) = 0;
  virtual [[nodiscard]] bool HasSpace() const = 0;
  virtual [[nodiscard]] uint16_t CalculateCrc16() const = 0;
  virtual [[nodiscard]] void Reset() = 0;
};

template <size_t size> class PacketBuffer : public IPacketBuffer, private NonCopyableAndNonMovable {
  static_assert(size > 0);

private:
  std::array<char, size> buffer;
  size_t packetSize = 0;

public:
  void Add(const char byte) override {
    buffer[packetSize] = byte;
    packetSize++;
  }

  void Reset() override { packetSize = 0; }

  [[nodiscard]] bool HasSpace() const override { return packetSize < buffer.size(); }

  [[nodiscard]] uint16_t CalculateCrc16() const override {
    uint16_t crc = 0;
    auto length = packetSize;
    const char* data = buffer.data();
    while (length--) {
      crc ^= static_cast<char>(*data++);
      for (int i = 0; i < 8; ++i) {
        if (crc & 1)
          crc = (crc >> 1) ^ 0xa001;
        else
          crc = (crc >> 1);
      }
    }
    return crc;
  }

  [[nodiscard]] StringView Data() const override { return StringView(buffer.data(), packetSize); }
};

class WaitingForPacketStartSymbol : public IState, private NonCopyableAndNonMovable {
private:
  IStateContext& ctx;
  IPacketBuffer& buf;

public:
  WaitingForPacketStartSymbol(IStateContext& ctx, IPacketBuffer& buf) : ctx(ctx), buf(buf) {}

private:
  const IPacket* ProcessByte(const char /* byte */) override { return nullptr; }
};

class WaitingForPacketEndSymbol : public IState, private NonCopyableAndNonMovable {
private:
  IStateContext& ctx;
  IPacketBuffer& buf;

public:
  WaitingForPacketEndSymbol(IStateContext& ctx, IPacketBuffer& buf) : ctx(ctx), buf(buf) {}

private:
  const IPacket* ProcessByte(const char byte) override {
    buf.Add(byte);

    if (byte == '!') {
      ctx.SetState(StateId::WaitingForCrc);
    }

    return nullptr;
  }
};

class WaitingForCrc : public IState, private NonCopyableAndNonMovable {
private:
  IStateContext& ctx;
  IPacketBuffer& buf;
  uint16_t crc = 0;
  size_t amountOfCrcBytesReceived = 0;

public:
  WaitingForCrc(IStateContext& ctx, IPacketBuffer& buf) : ctx(ctx), buf(buf) {}

  void Reset() { amountOfCrcBytesReceived = 0; }

private:
  const IPacket* ProcessByte(const char byte) override {
    if (!IsCrcByte(byte)) {
      ctx.SetState(StateId::WaitingForPacketStartSymbol);
      return nullptr;
    }

    AddToCrc(byte);

    if (amountOfCrcBytesReceived != 4) {
      return nullptr;
    }

    ctx.SetState(StateId::WaitingForPacketStartSymbol);

    if (crc == buf.CalculateCrc16()) {
      return &buf;
    } else {
      return nullptr;
    }
  }

  [[nodiscard]] static bool IsCrcByte(const char byte) { return (byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'F'); }

  void AddToCrc(char byte) {
    if (byte >= '0' && byte <= '9') {
      byte = byte - '0';
    } else if (byte >= 'A' && byte <= 'F') {
      byte = byte - 'A' + 10;
    }

    crc = (crc << 4) | (byte & 0xF);
    amountOfCrcBytesReceived++;
  }
};

template <size_t BufferSize> class DsmrPacketReceiver : public IStateContext, private NonCopyableAndNonMovable {
  PacketBuffer<BufferSize> buf;
  WaitingForPacketStartSymbol waitingForPacketStartSymbol{*this, buf};
  WaitingForPacketEndSymbol waitingForPacketEndSymbol{*this, buf};
  WaitingForCrc waitingForCrc{*this, buf};
  IState* currentState = &waitingForPacketStartSymbol;

public:
  const IPacket* ProcessByte(const char byte) {
    if (byte == '/') {
      buf.Reset();
      buf.Add(byte);
      SetState(StateId::WaitingForPacketEndSymbol);
      return nullptr;
    }

    if (!buf.HasSpace()) {
      buf.Reset();
      SetState(StateId::WaitingForPacketStartSymbol);
    }

    return currentState->ProcessByte(byte);
  }

private:
  void SetState(const StateId stateId) override {
    switch (stateId) {
    case DsmrParser::StateId::WaitingForPacketStartSymbol:
      currentState = &waitingForPacketStartSymbol;
      break;
    case DsmrParser::StateId::WaitingForPacketEndSymbol:
      currentState = &waitingForPacketEndSymbol;
      break;
    case DsmrParser::StateId::WaitingForCrc:
      waitingForCrc.Reset();
      currentState = &waitingForCrc;
      break;
    }
  }
};

struct DsmrPacketHeader {
  char version[4];
  StringView identification;
};

struct ObisCode {
  uint8_t A; // Medium: (0=abstract objects, 1=electricity, 6=heat, 7=gas, 8=water ...)
  uint8_t B; // Channel
  uint8_t C; // Group: the physical value (current, voltage, energy, level, temperature, ...)
  uint8_t D; // Quantity computation result of specific algorythm
  uint8_t E; // Measurement type defined by groups A to D into individual measurements (e.g. switching ranges)
};

struct DsmrDataObject {
  ObisCode obisCode;
  StringView value;
  StringView unit;
};

struct IDsmrParserResultReceiver {
  virtual void OnDsmrData(const DsmrDataObject& dsmrData) = 0;
};

class DsmrPacketParser : private NonCopyableAndNonMovable {
private:
  IDsmrParserResultReceiver& dataReceiver;

public:
  DsmrPacketParser(IDsmrParserResultReceiver& dataReceiver) : dataReceiver(dataReceiver) {}

#pragma warning(push)
#pragma warning(disable : 4101) // unreferenced local variable
#pragma warning(disable : 4189) // local variable is initialized but not referenced
#pragma warning(disable : 4701) // potentially uninitialized local variable
  [[nodiscard]] bool ParseHeader(const IPacket& packet, DsmrPacketHeader& header) {
    const char* YYCURSOR = (char*)packet.Data().Data();
    const char* YYMARKER;
    const char* YYLIMIT = YYCURSOR + packet.Data().Size();
    const char* t1;
    const char* t2;
    const char* t3;

    /*!stags:re2c format = 'const char *@@;\n'; */
    /*!re2c
        re2c:define:YYCTYPE = char;
        re2c:yyfill:enable = 0;
        re2c:tags = 1;

        [/] @t1 .{4} @t2 .+ @t3 [\r][\n] {
          strncpy(header.version, t1, 4);
          header.identification = StringView(t2, t3 - t2);
          return true;
        }
        * { return false; }
    */
  }

  void Parse(const IPacket& packet) {
    const char* YYCURSOR = (char*)packet.Data().Data();
    const char* YYMARKER;
    const char* YYLIMIT = YYCURSOR + packet.Data().Size();
    const char* t1 = nullptr;
    const char* t2 = nullptr;
    const char* t3 = nullptr;
    const char* t4 = nullptr;
    const char* t5 = nullptr;
    const char* t6 = nullptr;
    const char* t7 = nullptr;
    const char* t8 = nullptr;
    const char* t9 = nullptr;
    const char* t10 = nullptr;
    const char* t11 = nullptr;
    const char* t12 = nullptr;
    const char* t13 = nullptr;
    const char* t14 = nullptr;

    for (;;) {
      /*!stags:re2c format = 'const char *@@;\n'; */
      /*!re2c
          re2c:define:YYCTYPE = char;
          re2c:yyfill:enable = 0;
          re2c:tags = 1;
      
          obisCode = @t1 [0-9]+ @t2 [-] @t3 [0-9]+ @t4 [:] @t5 [0-9]+ @t6 [.] @t7 [0-9]+ @t8 [.] @t9 [0-9]+ @t10;
          value = @t11 [0-9.]+ @t12;
          unit = @t13 [a-zA-Z0-9]+ @t14;
      
          obisCode [(] value ([*] unit)? [)][\r][\n] {
            DsmrDataObject dsmrData;
            dsmrData.obisCode.A = StringToNumber(t1, t2);
            dsmrData.obisCode.B = StringToNumber(t3, t4);
            dsmrData.obisCode.C = StringToNumber(t5, t6);
            dsmrData.obisCode.D = StringToNumber(t7, t8);
            dsmrData.obisCode.E = StringToNumber(t9, t10);
            dsmrData.value = StringView(t11, t12 - t11);
            if (t13 != nullptr) {
              dsmrData.unit = StringView(t13, t14 - t13);
            }
            dataReceiver.OnDsmrData(dsmrData);
          }
          [\!] { return; }
          * { continue; }
      */
    }
  }
#pragma warning(pop)

  static uint8_t StringToNumber(const char* startPosition, const char* endPosition) {
    uint8_t n = 0;
    for (; startPosition < endPosition; startPosition++) {
      n = n * 10 + (*startPosition - '0');
    }
    return n;
  }
};

}
