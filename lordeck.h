#ifndef LOR_DECK_CODES_H
#define LOR_DECK_CODES_H

#include <cassert>
#include <climits>
#include <cstring>
#include <string>
#include <vector>
#include <regex>
#include <map>

#define MAX_KNOWN_VERSION 2
#define MASK 31
#define SHIFT 5
#define MSB 0x80
#define ALL_BUT_MSB 0x7f;

namespace lor {
typedef std::uint8_t byte;

static std::map<char, int32_t> CHAR_MAP = {
    {'A', 0}, {'B', 1}, {'C', 2}, {'D', 3},
    {'E', 4}, {'F', 5}, {'G', 6}, {'H', 7},
    {'I', 8}, {'J', 9}, {'K', 10}, {'L', 11},
    {'M', 12}, {'N', 13}, {'O', 14}, {'P', 15},
    {'Q', 16}, {'R', 17}, {'S', 18}, {'T', 19},
    {'U', 20}, {'V', 21}, {'W', 22}, {'X', 23},
    {'Y', 24}, {'Z', 25}, {'2', 26}, {'3', 27},
    {'4', 28}, {'5', 29}, {'6', 30}, {'7', 31}
};
static std::map<int32_t, std::string> FAC_MAP = {
    {0, "DE"}, {1, "FR"}, {2, "IO"}, {3, "NX"}, {4, "PZ"}, {5, "SI"}, {6, "BW"}
};

static char CHARS[] = {
    65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88,
    89, 90, 50, 51, 52, 53, 54, 55, 0};

static std::string padLeft(std::string input, std::string val, int32_t objLen) {
  int32_t inLen = input.size();
  int32_t valLen = input.size();
  int32_t times = (objLen - inLen) % valLen == 0 ? (objLen - inLen) / valLen : (objLen - inLen) / valLen + 1;
  int32_t outLen = times * valLen + inLen;
  const char *input_cstr = input.c_str();
  const char *val_cstr = val.c_str();
  char *buffer = new char[outLen + 1];
  for (int32_t i = 0; i < times; i++)
    std::memcpy(buffer + i * valLen, val_cstr, valLen);
  std::memcpy(buffer + times * valLen, input_cstr, inLen + 1);
  std::string res(buffer);
  delete[] buffer;
  return res;
}

class Base32 {
public:
  static std::vector<byte> decode(std::string code) {
    std::string encoded = code;
    std::regex p1("-");
    std::regex p2("^\\s+");
    std::regex p3("\\s+$");
    std::regex p4("[=]*$");
    encoded = std::regex_replace(encoded, p1, "");
    encoded = std::regex_replace(encoded, p2, "");
    encoded = std::regex_replace(encoded, p3, "");
    encoded = std::regex_replace(encoded, p4, "");
    for (int32_t i = 0; i < encoded.size(); i++)
      if (encoded[i] >= 97 && encoded[i] <= 122)
        encoded[i] -= 32;

    if (encoded.size() == 0)
      return std::vector<byte>();

    int32_t encLen = encoded.length();
    int32_t outLen = encLen * SHIFT / 8;
    std::vector<byte> result(outLen, 0);

    int32_t buff = 0;
    int32_t next = 0;
    int32_t bitsLeft = 0;

    for (char c : encoded) {
      if (CHAR_MAP.find(c) == CHAR_MAP.end())
        return std::vector<byte>();
      buff <<= SHIFT;
      buff |= (CHAR_MAP[c] & MASK);
      bitsLeft += SHIFT;
      if (bitsLeft >= 8) {
        result[next++] = (byte) (buff >> (bitsLeft - 8));
        bitsLeft -= 8;
      }
    }
    return result;
  }

  static std::string encode(std::vector<byte> data) {
    if (data.empty())
      return "";

    int32_t outLen = (data.size() * 8 + SHIFT - 1) / SHIFT;
    char *res_cs = new char[outLen + 1];
    res_cs[outLen] = 0;

    int32_t buff = data[0];
    int32_t next = 1;
    int32_t bitsLeft = 8;
    int32_t ri = 0;
    while (bitsLeft > 0 || next < data.size()) {
      if (bitsLeft < SHIFT) {
        if (next < data.size()) {
          buff <<= 8;
          buff |= (data[next++] & 0xff);
          bitsLeft += 8;
        } else {
          int32_t pad = SHIFT - bitsLeft;
          buff <<= pad;
          bitsLeft += pad;
        }
      }
      int32_t index = MASK & (buff >> (bitsLeft - SHIFT));
      bitsLeft -= SHIFT;
      res_cs[ri++] = CHARS[index];
    }
    std::string res(res_cs);
    delete[] res_cs;
    return res;
  }
};

class VarInt {
public:
  static int32_t pop(std::vector<byte> &bytes) {
    int32_t result = 0;
    int32_t shift = 0;
    int32_t popped = 0;
    for (int32_t i = 0; i < bytes.size(); i++) {
      popped++;
      int32_t current = bytes[i] & ALL_BUT_MSB;
      result |= current << shift;
      if ((bytes[i] & MSB) != MSB) {
        bytes.erase(bytes.begin(), bytes.begin() + popped);
        return result;
      }
      shift += 7;
    }
    return INT32_MIN;
  }

  static std::vector<int32_t> get(int32_t value) {
    std::vector<int32_t> buff(10, 0);
    int32_t index = 0;
    if (value == 0)
      return {0};
    while (value != 0) {
      int32_t byteVal = ((uint32_t) value) & ALL_BUT_MSB;
      value >>= 7;
      if (value != 0)
        byteVal |= MSB;

      buff[index++] = byteVal;
    }
    return std::vector<int32_t>(buff.begin(), buff.begin() + index);
  }
};

class LoRDeckCode {
public:
  static std::map<std::string, int32_t> decode(std::string code) {
    std::map<std::string, int32_t> deck;
    std::vector<byte> bytes = Base32::decode(code);
    byte firstByte = bytes.front();
    bytes.erase(bytes.begin());
    int32_t version = firstByte & 0xF;

    if (version > MAX_KNOWN_VERSION)
      return deck;

    for (int32_t i = 3; i > 0; i--) {
      int32_t groupCount = VarInt::pop(bytes);

      for (int32_t j = 0; j < groupCount; j++) {
        int32_t itemCount = VarInt::pop(bytes);
        int32_t set = VarInt::pop(bytes);
        int32_t faction = VarInt::pop(bytes);

        for (int32_t k = 0; k < itemCount; k++) {
          int32_t card = VarInt::pop(bytes);
          std::string setStr = padLeft(std::to_string(set), "0", 2);
          std::string facStr = FAC_MAP[faction];
          std::string numStr = padLeft(std::to_string(card), "0", 3);
          std::string cardCode = setStr + facStr + numStr;
          deck[cardCode] = i;
        }
      }
    }

    while (bytes.size() > 0) {
      int32_t count = VarInt::pop(bytes);
      int32_t set = VarInt::pop(bytes);
      int32_t faction = VarInt::pop(bytes);
      int32_t number = VarInt::pop(bytes);

      std::string setStr = padLeft(std::to_string(set), "0", 2);
      std::string facStr = FAC_MAP[faction];
      std::string numStr = padLeft(std::to_string(number), "0", 3);
      std::string cardCode = setStr + facStr + numStr;
      deck[cardCode] = count;
    }
    return deck;
  }
};
}
#endif //LOR_DECK_CODES_H
