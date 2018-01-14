#include "vega/date.h"
#include "vega/vega.h"

#include <sstream>

#include "vega/regex_string.h"

namespace vega {
  const std::shared_ptr<const std::regex> Date::SINGLE_DATE_REGEX = std::make_shared<const std::regex>(
    "\\d{4}(\\.?(0[1-9]|1[0-2])(\\.?(0[1-9]|[1-2]\\d|3[0-1]))?)?"
  );
  const std::shared_ptr<const std::regex> Date::DATE_RANGE_REGEX = std::make_shared<const std::regex>(
    "\\s*([\\.\\d]*)\\s*-\\s*([\\.\\d]*)\\s*"
  );

  Date::Date(const std::string& s) {
    this->set_string(s);
  }

  void Date::set_string(const std::string& s) {
    std::smatch match;

    // Is Date range
    if (std::regex_search(s.begin(), s.end(), match, *Date::DATE_RANGE_REGEX)) {
      std::string lower = match[1].str();
      if (lower.size() > 0) {
        m_lower = std::make_shared<const Date>(lower);
      }

      std::string upper = match[2].str();
      if (upper.size() > 0) {
        m_upper = std::make_shared<const Date>(upper);
      }
    }
    // Is single Date
    else {
      m_date = std::make_shared<const RegexString>(s, Date::SINGLE_DATE_REGEX);
    }
  }

  Date::Date(const std::shared_ptr<Date>& lower, const std::shared_ptr<Date>& upper)
    :
      m_lower(std::move(lower)),
      m_upper(std::move(upper))
  {
  }

  bool Date::is_range() const { return m_lower || m_upper; }
  const std::shared_ptr<const Date>& Date::lower() const { return m_lower; }
  const std::shared_ptr<const Date>& Date::upper() const { return m_upper; }

  bool Date::operator==(const Date& other) const {
    return this->str() == other.str();
  }

  bool Date::operator!=(const Date& other) const {
    return !(*this == other);
  }

  std::string Date::str() const {
    if (this->is_range()) {
      std::stringstream ss;
      if (m_lower) ss << m_lower->str();
      ss << '-';
      if (m_upper) ss << m_upper->str();
      return ss.str();
    }
    else {
      return m_date->str();
    }
  }
}