#pragma once

#include <glm/ext.hpp>

namespace boost
{
namespace serialization
{
template <typename Archive>
void serialize(Archive &ar, glm::dvec2 vec, unsigned int)
{
	ar &vec.x;
	ar &vec.y;
}
} // namespace serialization
} // namespace boost
