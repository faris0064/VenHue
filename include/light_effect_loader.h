#pragma once

#include "animated_effect_definition.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

using AnimatedEffectDefinitions = std::map<std::string, AnimatedEffectDefinition, std::less<>>;

AnimatedEffectDefinitions loadAnimatedEffectDefinitions(const std::vector<std::string_view> &effectNames);
