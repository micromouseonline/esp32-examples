#pragma once

/// @file beacon-frame.h
/// @brief Raw 802.11 beacon frame construction for esp_wifi_80211_tx().
///
/// Transmits real 2.4GHz RF at high packet rate. Only use against your own
/// equipment on spectrum/premises you are authorized to test on.

#include <cstdint>
#include <cstring>

#include <esp_random.h>

namespace beacon_frame {

constexpr size_t kMaxSsidLen = 32;
constexpr size_t kMacHeaderLen = 24;
constexpr size_t kFixedFieldsLen = 12;  // timestamp(8) + interval(2) + capability(2)
constexpr size_t kRatesIeLen = 10;      // id(1) + len(1) + 8 rate bytes
constexpr size_t kDsParamIeLen = 3;     // id(1) + len(1) + channel(1)

/// Upper bound on a built frame's length (max-length SSID), used to size buffers.
constexpr size_t kMaxFrameLen = kMacHeaderLen + kFixedFieldsLen + (2 + kMaxSsidLen) + kRatesIeLen + kDsParamIeLen;

/// Fills mac[6] with a random, locally-administered, unicast MAC address.
inline void randomLocallyAdministeredMac(uint8_t mac[6]) {
  esp_fill_random(mac, 6);
  mac[0] = (mac[0] & 0xFC) | 0x02;  // clear multicast bit, set locally-administered bit
}

/// Offset of the DS Parameter Set channel byte within a frame built with the given SSID length.
/// Lets callers patch just the channel byte in place after a channel change, without rebuilding.
inline size_t dsParamChannelOffset(size_t ssidLen) {
  return kMacHeaderLen + kFixedFieldsLen + (2 + ssidLen) + kRatesIeLen + 2;
}

/// Assembles one complete beacon frame into out (must hold at least kMaxFrameLen bytes).
/// Returns the frame length in bytes.
inline size_t buildBeaconFrame(uint8_t* out, const uint8_t mac[6], const char* ssid, uint8_t channel) {
  size_t ssidLen = strlen(ssid);
  if (ssidLen > kMaxSsidLen) {
    ssidLen = kMaxSsidLen;
  }

  size_t offset = 0;

  // Frame Control: type=Management(00), subtype=Beacon(1000)
  out[offset++] = 0x80;
  out[offset++] = 0x00;
  // Duration/ID
  out[offset++] = 0x00;
  out[offset++] = 0x00;
  // Address1: broadcast destination
  memset(out + offset, 0xFF, 6);
  offset += 6;
  // Address2: source (fake AP MAC)
  memcpy(out + offset, mac, 6);
  offset += 6;
  // Address3: BSSID (same as source)
  memcpy(out + offset, mac, 6);
  offset += 6;
  // Sequence/fragment control -- overwritten by the driver since esp_wifi_80211_tx is
  // called with en_sys_seq=true; the value here is a genuine don't-care.
  out[offset++] = 0x00;
  out[offset++] = 0x00;

  // Timestamp -- zero. TSF correctness is irrelevant for scan-list flooding / airtime use.
  memset(out + offset, 0x00, 8);
  offset += 8;
  // Beacon interval: 100 TU, little-endian
  out[offset++] = 0x64;
  out[offset++] = 0x00;
  // Capability info: ESS + Privacy, little-endian 0x0011
  out[offset++] = 0x11;
  out[offset++] = 0x00;

  // SSID IE
  out[offset++] = 0x00;
  out[offset++] = (uint8_t)ssidLen;
  memcpy(out + offset, ssid, ssidLen);
  offset += ssidLen;

  // Supported Rates IE: 1, 2, 5.5, 11 (mandatory) + 18, 24, 36, 54 Mbps (optional)
  static const uint8_t kRates[8] = {0x82, 0x84, 0x8B, 0x96, 0x24, 0x30, 0x48, 0x6C};
  out[offset++] = 0x01;
  out[offset++] = sizeof(kRates);
  memcpy(out + offset, kRates, sizeof(kRates));
  offset += sizeof(kRates);

  // DS Parameter Set IE: claimed channel. Must always match the radio's actual
  // esp_wifi_set_channel() value -- callers patch this byte in place on channel change.
  out[offset++] = 0x03;
  out[offset++] = 0x01;
  out[offset++] = channel;

  return offset;
}

}  // namespace beacon_frame
