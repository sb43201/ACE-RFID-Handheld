#pragma once

#include "AceTag.h"

namespace AceCodec {
bool validateAndDecode(AceTagData &tag);
void dump(const AceTagData &tag);
uint16_t rgb565(const AceTagData &tag);
uint16_t contrastText(const AceTagData &tag);
}  // namespace AceCodec

