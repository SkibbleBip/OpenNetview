#ifndef __NV_HELPER_HPP__
#define __NV_HELPER_HPP__

#include <variant>

namespace OpenNetview {

template <typename GoodType, typename FailType> class Result {
private:
  bool isFailure;

public:
  std::variant<GoodType, FailType> data;

  bool is_fail(void) const { return isFailure; }

  Result(GoodType &&in) : isFailure(false), data(std::move(in)) {}

  Result(FailType in) : isFailure(true), data(in) {}

  Result(const Result &) = delete;
  Result &operator=(const Result &) = delete;

  Result(Result &&other) noexcept = default;
  Result &operator=(Result &&other) noexcept = default;

  GoodType &getSuccess(void) { return std::get<GoodType>(data); }
  FailType &getFailure(void) { return std::get<FailType>(data); }
};

} // namespace OpenNetview

#endif
