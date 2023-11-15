#ifndef SRC_BASE_NONCOPYABLE_H_
#define SRC_BASE_NONCOPYABLE_H_


namespace chml {

class noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}

 private:
  // emphasize the following members are private
  noncopyable(const noncopyable&);
  const noncopyable& operator=(const noncopyable&);
};
}  // namespace chml

#endif  // SRC_BASE_NONCOPYABLE_H_
