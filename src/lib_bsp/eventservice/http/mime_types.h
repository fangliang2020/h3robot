#ifndef HTTP_SERVER4_MIME_TYPES_HPP
#define HTTP_SERVER4_MIME_TYPES_HPP

#include <string>

namespace chml {

/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);

}  // namespace chml

#endif  // HTTP_SERVER4_MIME_TYPES_HPP

