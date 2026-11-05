#include "map_internal.h"
namespace huxerui::map {
bool IsSupported(Provider) noexcept { return false; }
void detail::InstallPlatformMaps(ApplicationContext&) {}
} // namespace huxerui::map
