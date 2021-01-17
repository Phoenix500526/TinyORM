#ifndef TINYORM_H_
#define TINYORM_H_

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#define REFLECTION(_TABLE_NAME_, ...)                                     \
 private:                                                             \
  friend class tinyorm_impl::ReflectionVisitor;                         \
  template <typename Fn>                                              \
  inline auto __Apply(const Fn& fn) {                                 \
    return fn(__VA_ARGS__);                                           \
  }                                                                   \
  template <typename Fn>											  \
  inline auto __Apply(const Fn& fn) const { return fn(__VA_ARGS__); } \
  constexpr static const char* __TableName = _TABLE_NAME_;            \
  constexpr static const char* __FieldNames = #__VA_ARGS__;	//!< Generate reflection information for user-defined class

#define NO_REFLECTIONED \
  "Please Inject the metainformation of your class by `REFLECTION` first"
#define NO_SUCH_FIELD \
  "No such field"

namespace tinyorm_impl {
/**
 * @brief A visitor for reflection
 * @details User shouldn't access the reflection information directly, cuz that will spoil the encapsulation.
 */
class ReflectionVisitor {
 private:
  static std::vector<std::string> ExtractFieldName(std::string_view input) {
    std::vector<std::string> ret;
    std::string::size_type start = 0, comma = 0;
    while (comma != std::string_view::npos) {
      comma = input.find_first_of(',', start);
      if (__builtin_expect(input[start] == ' ', 0)) {
        ++start;
      }
      if (__builtin_expect(input[comma - 1] == ' ', 0)) {
        ret.emplace_back(input.substr(start, comma - start - 1));
      } else {
        ret.emplace_back(input.substr(start, comma - start));
      }
      start = comma + 1;
    }
    return ret;
  }

 public:
  template <typename T>
  class HasInjected {
   private:
    template <typename, typename = std::void_t<>>
    struct Check : std::false_type {};
    template <typename U>
    struct Check<U, std::void_t<decltype(U::__TableName)>> : std::true_type {};

   public:
    constexpr static bool value = Check<T>::value;
    static_assert(value, NO_REFLECTIONED);
  };

  template <typename C>
  inline static const std::string& TableName(const C&) {
    static const std::string tableName(C::__TableName);
    return tableName;
  }

  template <typename C>
  inline static const std::vector<std::string>& FieldNames(const C&){
  	static const auto fieldNames = ExtractFieldName(C::__FieldNames);
  	return fieldNames;
  }

  template <typename C, typename Fn>
  inline static auto Visit(C& obj, const Fn& f){
  	return obj.__Apply(f);
  }
};

}  // namespace tinyorm_impl


namespace tinyorm{
/**
 * @brief Extract fields from the given object for the JOIN operation.
 */
class FieldExtractor
{
private:
	using pair_type = std::pair<const std::string&, const std::string&>;
	template <typename C>
	using HasInjected = tinyorm_impl::ReflectionVisitor::HasInjected<C>;
	std::unordered_map<const void *, pair_type> fieldCache_;

	template <typename T>
	const pair_type& Get(const T& field) const{
		try{
			return fieldCache_.at((const void*)&field);
		}catch(...){
			throw(std::runtime_error(NO_SUCH_FIELD));
		}
	}

	template <typename C>
	std::enable_if_t<!HasInjected<C>::value> Extract(const C&){}

	template <typename C>
	std::enable_if_t<HasInjected<C>::value> Extract(const C& obj){
		tinyorm_impl::ReflectionVisitor::Visit(obj, 
			[this, &obj](const auto&... args){
				const auto& tableName = tinyorm_impl::ReflectionVisitor::TableName(obj);
				const auto& fieldNames = tinyorm_impl::ReflectionVisitor::FieldNames(obj);
				int index = 0;
				(fieldCache_.emplace(&args, pair_type{fieldNames[index++], tableName}),...);
			}
		);
	}

public:
	template <typename... Classes>
	FieldExtractor(const Classes&... args){
		((Extract(args)), ...);
	}
	~FieldExtractor() = default;
	
	template <typename T>
	inline const pair_type& operator()(const T& field) const{
		return Get(field);
	}
};
}  // namespace tinyorm

#undef NO_SUCH_FIELD
#undef NO_REFLECTIONED
#endif  // TINYORM_H_
