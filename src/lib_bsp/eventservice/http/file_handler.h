#ifndef HTTP_SERVER4_FILE_HANDLER_HPP
#define HTTP_SERVER4_FILE_HANDLER_HPP

#include <string>
#include "eventservice/http/handlermanager.h"

namespace chml {

struct reply;

/// The common handler for all incoming requests.
class file_handler : public HttpHandler {
 public:
  /// Construct with a directory containing files to be served.
  explicit file_handler(EventService::Ptr event_service,
                        const std::string& doc_root);
  /// Handle a request and produce a reply.
  virtual bool HandleRequest(AsyncHttpSocket::Ptr connect,
                             HttpReqMessage& request);

 private:
  /// The directory containing the files to be served.
  std::string doc_root_;
  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

}  // namespace chml

#endif  // HTTP_SERVER4_FILE_HANDLER_HPP

