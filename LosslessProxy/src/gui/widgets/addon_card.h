#pragma once
#include "../../addon/addon_info.h"
#include "imgui.h"

namespace lsproxy {
namespace widgets {

// Renders a single addon card. Returns true if the card was clicked (selected).
bool AddonCard(AddonInfo& addon, int index, bool isSelected);

} // namespace widgets
} // namespace lsproxy
