/// Bela Mapview feature
#ifndef BELA_MAPVIEW_HPP
#define BELA_MAPVIEW_HPP
#include "base.hpp"

namespace bela {
class MemView {
public:
  static constexpr size_t npos = static_cast<size_t>(-1);
  MemView() = default;
  template <typename T>
  MemView(const T *d, size_t l)
      : data_(reinterpret_cast<const uint8_t *>(d)), size_(l * sizeof(T)) {}
  MemView(const MemView &other) {
    data_ = other.data_;
    size_ = other.size_;
  }
  template <typename T, size_t ArrayLen>
  bool StartsWith(const T (&bv)[ArrayLen]) const {
    return ArrayLen * sizeof(T) <= size_ &&
           (memcmp(data_, bv, ArrayLen * sizeof(T)) == 0);
  }
  template <typename T> bool StartsWith(std::basic_string_view<T> sv) const {
    return sv.size() * sizeof(T) <= size_ &&
           (memcmp(data_, sv.data(), sv.size() * sizeof(T)) == 0);
  }

  bool StartsWith(MemView mv) const {
    return mv.size_ <= size_ && (memcmp(data_, mv.data_, mv.size_) == 0);
  }

  template <typename T, size_t ArrayLen>
  bool IndexsWith(size_t pos, const T (&bv)[ArrayLen]) const {
    return ArrayLen * sizeof(T) + pos <= size_ &&
           (memcmp(data_ + pos, bv, ArrayLen * sizeof(T)) == 0);
  }
  template <typename T>
  bool IndexsWith(size_t pos, std::basic_string_view<T> sv) const {
    return sv.size() * sizeof(T) + pos <= size_ &&
           (memcmp(data_ + pos, sv.data(), sv.size() * sizeof(T)) == 0);
  }

  bool IndexsWith(size_t pos, MemView mv) const {
    return mv.size_ + pos <= size_ &&
           (memcmp(data_ + pos, mv.data_, mv.size_) == 0);
  }

  MemView submv(std::size_t pos, std::size_t n = npos) {
    return MemView(data_ + pos, (std::min)(n, size_ - pos));
  }
  std::size_t size() const { return size_; }
  const uint8_t *data() const { return data_; }
  std::string_view sv() const {
    return std::string_view(reinterpret_cast<const char *>(data_), size_);
  }
  unsigned char operator[](const std::size_t off) const {
    if (off >= size_) {
      return UCHAR_MAX;
    }
    return (unsigned char)data_[off];
  }
  template <typename T> const T *cast(size_t off) const {
    if (off + sizeof(T) >= size_) {
      return nullptr;
    }
    return reinterpret_cast<const T *>(data_ + off);
  }

private:
  const uint8_t *data_{nullptr};
  size_t size_{0};
};

// MapView mean this memory is readonly!!!
class MapView {
private:
  static inline void closeauto(HANDLE hFile) {
    if (hFile != INVALID_HANDLE_VALUE) {
      CloseHandle(hFile);
    }
  }

public:
  static constexpr size_t npos = static_cast<size_t>(-1);
  MapView() = default;
  MapView(const MapView &) = delete;
  MapView &operator=(const MapView &) = delete;
  ~MapView() {
    if (data_ != nullptr) {
      ::UnmapViewOfFile(data_);
    }
    closeauto(FileMap);
    closeauto(FileHandle);
  }
  bool MappingView(std::wstring_view file, bela::error_code &ec,
                   std::size_t minsize = 1, std::size_t maxsize = SIZE_MAX);
  MemView subview(size_t off = 0) const {
    if (off >= size_) {
      return MemView();
    }
    return MemView(data_ + off, size_ - off);
  }

private:
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
  HANDLE FileMap{INVALID_HANDLE_VALUE};
  uint8_t *data_{nullptr};
  std::size_t size_{0};
};

inline bool MapView::MappingView(std::wstring_view file, bela::error_code &ec,
                                 std::size_t minsize, std::size_t maxsize) {
  if ((FileHandle = CreateFileW(file.data(), GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                nullptr)) == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  LARGE_INTEGER li;
  if (GetFileSizeEx(FileHandle, &li) != TRUE ||
      (std::size_t)li.QuadPart < minsize) {
    ec = bela::make_error_code(bela::FileSizeTooSmall,
                               L"File size too smal, size: ", li.QuadPart);
    return false;
  }
  if ((FileMap = CreateFileMappingW(FileHandle, nullptr, PAGE_READONLY, 0, 0,
                                    nullptr)) == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  size_ = (size_t)li.QuadPart > maxsize ? maxsize : (size_t)li.QuadPart;
  auto baseAddr = MapViewOfFile(FileMap, FILE_MAP_READ, 0, 0, size_);
  if (baseAddr == nullptr) {
    ec = bela::make_system_error_code();
    return false;
  }
  data_ = reinterpret_cast<uint8_t *>(baseAddr);
  return true;
}

} // namespace bela

#endif