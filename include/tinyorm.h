#ifndef TINYORM_H_
#define TINYORM_H_

#include <cstddef>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "db_base.h"

#define REFLECTION(_TABLE_NAME_, ...)                      \
 private:                                                  \
  friend class tinyorm_impl::ReflectionVisitor;            \
  template <typename Fn>                                   \
  inline auto __Apply(const Fn& fn) {                      \
    return fn(__VA_ARGS__);                                \
  }                                                        \
  template <typename Fn>                                   \
  inline auto __Apply(const Fn& fn) const {                \
    return fn(__VA_ARGS__);                                \
  }                                                        \
  constexpr static const char* __TableName = _TABLE_NAME_; \
  constexpr static const char* __FieldNames =              \
      #__VA_ARGS__;  //!< Generate reflection information for
                     //!< user-defined class

#define NO_REFLECTIONED \
  "Please Inject the metainformation of your class by `REFLECTION` first"
#define NO_SUCH_FIELD "No such a field"
#define BAD_TYPE "Invalid Type"
#define NULL_DESERIALIZE "Cannot deserialize NULL value to a non-nullable value"
#define NOT_THE_SAME_TABLE "Field is not in the same table"
#define BAD_COLUMN_COUNT "Bad Column Count"
namespace tinyorm {

/**
 * @brief Nullable is wrapper class.
 * @details
 *  - A Nullable object is a equivalent to the data whose value can be NULL in
 * database.
 *  - All the Nullable objects are value semantics, they are copyable,
 * moveable.
 */
template <typename T>
class Nullable {
 private:
  bool hasValue_;
  T value_;

  template <typename T2>
  friend bool operator==(const Nullable<T2>& op1, const Nullable<T2>& op2);
  template <typename T2>
  friend bool operator==(const Nullable<T2>& op1, const T2& op2);
  template <typename T2>
  friend bool operator==(const T2& op1, const Nullable<T2>& op2);
  template <typename T2>
  friend bool operator==(const Nullable<T2>& op1, std::nullptr_t);
  template <typename T2>
  friend bool operator==(std::nullptr_t, const Nullable<T2>& op2);

 public:
  Nullable() : hasValue_(false), value_(T()) {}
  Nullable(std::nullptr_t) : Nullable() {}
  Nullable(const T& value) : hasValue_(true), value_(value) {}
  ~Nullable() = default;

  Nullable<T> operator=(std::nullptr_t) {
    hasValue_ = false;
    value_ = T();
    return *this;
  }

  Nullable<T> operator=(const T& value) {
    hasValue_ = true;
    value_ = value;
    return *this;
  }

  inline bool HasValue() const { return hasValue_; }

  inline const T& Value() const { return value_; }
};

template <typename T2>
bool operator==(const Nullable<T2>& op1, const Nullable<T2>& op2) {
  return (!op1.HasValue() && !op2.HasValue()) ||
         ((op1.HasValue() == op2.HasValue()) && (op1.Value() == op2.Value()));
}

template <typename T2>
bool operator==(const Nullable<T2>& op1, const T2& op2) {
  return op1.HasValue() && op1.Value() == op2;
}

template <typename T2>
bool operator==(const T2& op1, const Nullable<T2>& op2) {
  return operator==(op2, op1);
}

template <typename T2>
bool operator==(const Nullable<T2>& op1, std::nullptr_t) {
  return !op1.HasValue();
}

template <typename T2>
bool operator==(std::nullptr_t, const Nullable<T2>& op2) {
  return operator==(op2, nullptr);
}

}  // namespace tinyorm

namespace tinyorm {
class DBManager;
}

namespace tinyorm_impl {
/**
 * @brief A visitor for reflection
 * @details User shouldn't access the reflection information directly, cuz that
 * will spoil the encapsulation.
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
  inline static const std::vector<std::string>& FieldNames(const C&) {
    static const auto fieldNames = ExtractFieldName(C::__FieldNames);
    return fieldNames;
  }
  /**
   * @brief Vistor can iterate over the field names which defined in a class
   * @details [long description]
   *
   * @param obj
   * @param f f is a callable object which receives a variadic parameter list.
   */
  template <typename C, typename Fn>
  inline static auto Visit(C& obj, const Fn& f) {
    return obj.__Apply(f);
  }
};

/**
 * @brief Mapping C++ data types to SQL data types
 * @details TypeString will checking type string during compile time. It
 * supports three C++ data types:
 * - All integral but any char types.
 * - All floating type
 * - string type
 */
template <typename T>
struct TypeString {
  static constexpr const char* type_string =
      std::is_integral_v<T> && !std::is_same<T, char>::value &&
              !std::is_same<T, char16_t>::value &&
              !std::is_same<T, char32_t>::value &&
              !std::is_same<T, wchar_t>::value &&
              !std::is_same<T, unsigned char>::value
          ? " integer"
          : std::is_floating_point_v<T>
                ? " real"
                : std::is_same<T, std::string>::value ? " text" : nullptr;
  static_assert(type_string != nullptr, BAD_TYPE);
};

template <typename T>
struct TypeString<tinyorm::Nullable<T>> : TypeString<T> {};

class Serializer {
 public:
  template <typename T>
  inline static std::enable_if_t<TypeString<T>::type_string == nullptr, bool>
  Serialize(std::ostream& os, const T& value) {}

  template <typename T>
  inline static std::enable_if_t<TypeString<T>::type_string != nullptr, bool>
  Serialize(std::ostream& os, const T& value) {
    os << value;
    return true;
  }

  inline static bool Serialize(std::ostream& os, const std::string& value) {
    os << "'" << value << "'";
    return true;
  }

  template <typename T>
  inline static std::enable_if_t<TypeString<T>::type_string != nullptr, bool>
  Serialize(std::ostream& os, const tinyorm::Nullable<T>& value) {
    if (value.HasValue()) {
      return Serialize(os, value.Value());
    }
    return false;
  }
};

class Deserializer {
 public:
  template <typename T>
  inline static std::enable_if_t<TypeString<T>::type_string == nullptr, void>
  Deserialize(T&, const char*) {}

  template <typename T>
  inline static std::enable_if_t<TypeString<T>::type_string != nullptr, void>
  Deserialize(T& property, const char* value) {
    if (value) {
      std::istringstream{value} >> property;
    } else {
      throw std::runtime_error{NULL_DESERIALIZE};
    }
  }

  template <typename T>
  inline static std::enable_if_t<TypeString<T>::type_string != nullptr, void>
  Deserialize(std::string& property, const char* value) {
    if (value) {
      property = value;
    } else {
      throw std::runtime_error{NULL_DESERIALIZE};
    }
  }

  template <typename T>
  inline static std::enable_if_t<TypeString<T>::type_string != nullptr, void>
  Deserialize(tinyorm::Nullable<T>& property, const char* value) {
    if (value) {
      T res;
      Deserialize(res, value);
      property = res;
    } else {
      property = nullptr;
    }
  }
};

namespace Expression {

/**
 * @brief AssignmentExpr can serialize a C++ assignment expression, like
 * `name="phoenix"`, to a SQL expression,like name='phoenix'
 */
class AssignmentExpr {
 private:
  std::string expr_;

 public:
  AssignmentExpr(const std::string& op) : expr_(std::move(op)) {}
  ~AssignmentExpr() = default;
  const std::string& ToString() const { return expr_; }
  inline AssignmentExpr operator&&(const AssignmentExpr& rhs) {
    return AssignmentExpr{expr_ + "," + rhs.expr_};
  }
};

template <typename T>
struct FieldBase {
  std::string fieldName_;
  const std::string* tableName_;
  FieldBase(const std::string& field, const std::string* table)
      : fieldName_(std::move(field)), tableName_(table) {}
  inline std::string ToString() {
    return std::string{(tableName_ ? *tableName_ : "") + fieldName_};
  }
};

template <typename T>
struct Field : public FieldBase<T> {
  Field(const std::string& field, const std::string* table)
      : FieldBase<T>(field, table) {}
  inline AssignmentExpr operator=(T value) {
    std::ostringstream os;
    Serializer::Serialize(os << this->fieldName_ << "=", value);
    return AssignmentExpr(os.str());
  }
};

template <typename T>
struct NullableField : public Field<T> {
  NullableField(const std::string& field, const std::string* table)
      : Field<T>(std::move(field), table) {}
  inline AssignmentExpr operator=(T value) {
    std::ostringstream os;
    Serializer::Serialize(os << this->fieldName_ << "=", value);
    return AssignmentExpr(os.str());
  }
  inline AssignmentExpr operator=(std::nullptr_t) {
    return AssignmentExpr{this->fieldName_ + "=null"};
  }
};

template <typename T>
struct AggregateField : public FieldBase<T> {
  AggregateField(std::string function)
      : FieldBase<T>(std::move(function), nullptr) {}
  AggregateField(std::string function, const Field<T>& field)
      : FieldBase<T>(std::move(function) + "(" + *field.tableName_ + "." +
                         field.fieldName_ + ")",
                     nullptr) {}
};

class RelationExpr {
 public:
  /**
   * @brief Unary Relationship Expression Constructor
   */
  template <typename T>
  RelationExpr(const FieldBase<T>& field, std::string op)
      : exprs_{{field.fieldName_ + op, field.tableName_}} {}

  /**
   * @brief Binary Relationship Expression Constructor
   */
  template <typename T>
  RelationExpr(const FieldBase<T>& field, std::string op, T value) {
    std::ostringstream os;
    Serializer::Serialize(os << field.fieldName_ << op, value);
    exprs_.emplace_back(os.str(), field.tableName_);
  }

  /**
   * @brief Binary Relationship Expression Constructor
   */
  template <typename T>
  RelationExpr(const FieldBase<T>& lhs, std::string op, const FieldBase<T>& rhs)
      : exprs_{{lhs.fieldName_, lhs.tableName_},
               {std::move(op), nullptr},
               {rhs.fieldName_, rhs.tableName_}} {}

  std::string ToString() const {
    std::ostringstream os;
    for (const auto& item : exprs_) {
      if (item.second) os << *(item.second) << ".";
      os << item.first;
    }
    return os.str();
  }

  inline RelationExpr operator&&(const RelationExpr& rhs) const {
    return And_Or(rhs, " and ");
  }

  inline RelationExpr operator||(const RelationExpr& rhs) const {
    return And_Or(rhs, " or ");
  }

 private:
  std::list<std::pair<std::string, const std::string*>> exprs_;
  inline RelationExpr And_Or(const RelationExpr& rhs, std::string op) const {
    auto ret = *this;
    auto rightExprs = rhs.exprs_;
    ret.exprs_.emplace_front("(", nullptr);
    ret.exprs_.emplace_back(std::move(op), nullptr);
    ret.exprs_.splice(ret.exprs_.cend(), std::move(rightExprs));
    ret.exprs_.emplace_back(")", nullptr);
    return ret;
  }
};

//! Field op value
template <typename T>
inline RelationExpr operator==(const FieldBase<T>& field, T value) {
  return RelationExpr(field, "=", value);
}

template <typename T>
inline RelationExpr operator!=(const FieldBase<T>& field, T value) {
  return RelationExpr(field, "!=", value);
}

template <typename T>
inline RelationExpr operator<(const FieldBase<T>& field, T value) {
  return RelationExpr(field, "<", value);
}

template <typename T>
inline RelationExpr operator<=(const FieldBase<T>& field, T value) {
  return RelationExpr(field, "<=", value);
}

template <typename T>
inline RelationExpr operator>(const FieldBase<T>& field, T value) {
  return RelationExpr(field, ">", value);
}

template <typename T>
inline RelationExpr operator>=(const FieldBase<T>& field, T value) {
  return RelationExpr(field, ">=", value);
}

inline RelationExpr operator&(const FieldBase<std::string>& field,
                              const std::string& value) {
  return RelationExpr(field, " like ", value);
}

inline RelationExpr operator|(const FieldBase<std::string>& field,
                              const std::string& value) {
  return RelationExpr(field, " not like ", value);
}

//! Field op Field
template <typename T>
inline RelationExpr operator==(const FieldBase<T>& lhs,
                               const FieldBase<T>& rhs) {
  return RelationExpr(lhs, "=", rhs);
}

template <typename T>
inline RelationExpr operator!=(const FieldBase<T>& lhs,
                               const FieldBase<T>& rhs) {
  return RelationExpr(lhs, "!=", rhs);
}

template <typename T>
inline RelationExpr operator<(const FieldBase<T>& lhs,
                              const FieldBase<T>& rhs) {
  return RelationExpr(lhs, "<", rhs);
}

template <typename T>
inline RelationExpr operator<=(const FieldBase<T>& lhs,
                               const FieldBase<T>& rhs) {
  return RelationExpr(lhs, "<=", rhs);
}

template <typename T>
inline RelationExpr operator>(const FieldBase<T>& lhs,
                              const FieldBase<T>& rhs) {
  return RelationExpr(lhs, ">", rhs);
}

template <typename T>
inline RelationExpr operator>=(const FieldBase<T>& lhs,
                               const FieldBase<T>& rhs) {
  return RelationExpr(lhs, ">=", rhs);
}

//! NullableField op null
template <typename T>
inline RelationExpr operator==(const FieldBase<T>& lhs, std::nullptr_t) {
  return RelationExpr(lhs, " is null");
}

template <typename T>
inline RelationExpr operator!=(const FieldBase<T>& lhs, std::nullptr_t) {
  return RelationExpr(lhs, " is not null");
}

//! Aggregate Function
inline auto Count() { return AggregateField<size_t>("count (*)"); }

template <typename T>
inline auto Count(const Field<T>& field) {
  return AggregateField<T>("count", field);
}

template <typename T>
inline auto Sum(const Field<T>& field) {
  return AggregateField<T>("sum", field);
}

template <typename T>
inline auto Max(const Field<T>& field) {
  return AggregateField<T>("max", field);
}

template <typename T>
inline auto Min(const Field<T>& field) {
  return AggregateField<T>("min", field);
}

template <typename T>
inline auto Avg(const Field<T>& field) {
  return AggregateField<T>("avg", field);
}

}  // namespace Expression
}  // namespace tinyorm_impl

namespace tinyorm {
/**
 * @brief Extract fields from the given object for the JOIN operation.
 */
class FieldExtractor {
 private:
  using pair_type = std::pair<const std::string&, const std::string&>;
  template <typename C>
  using HasInjected = tinyorm_impl::ReflectionVisitor::HasInjected<C>;
  std::unordered_map<const void*, pair_type> fieldCache_;

  template <typename T>
  const pair_type& Get(const T& field) const {
    try {
      return fieldCache_.at((const void*)&field);
    } catch (...) {
      throw(std::runtime_error(NO_SUCH_FIELD));
    }
  }

  template <typename C>
  std::enable_if_t<!HasInjected<C>::value> Extract(const C&) {}

  template <typename C>
  std::enable_if_t<HasInjected<C>::value> Extract(const C& obj) {
    tinyorm_impl::ReflectionVisitor::Visit(obj, [this,
                                                 &obj](const auto&... args) {
      const auto& tableName = tinyorm_impl::ReflectionVisitor::TableName(obj);
      const auto& fieldNames = tinyorm_impl::ReflectionVisitor::FieldNames(obj);
      int index = 0;
      (fieldCache_.emplace(&args, pair_type{fieldNames[index++], tableName}),
       ...);
    });
  }

 public:
  template <typename... Classes>
  FieldExtractor(const Classes&... args) {
    (Extract(args), ...);
  }
  ~FieldExtractor() = default;

  template <typename T>
  inline tinyorm_impl::Expression::NullableField<T> operator()(
      const Nullable<T>& field) const {
    const auto& res = Get(field);
    return tinyorm_impl::Expression::NullableField<T>{std::move(res.first),
                                                      &res.second};
  }

  template <typename T>
  inline tinyorm_impl::Expression::Field<T> operator()(const T& field) const {
    const auto& res = Get(field);
    return tinyorm_impl::Expression::Field<T>{std::move(res.first),
                                              &res.second};
  }
};

/**
 * @brief SQL Constraint
 * @details NOT NULL. UNIQUE, PRIMARY KEY,  FOREIGN KEY, CHECK, DEFAULT
 */
class Constraint {
 private:
  std::string constraint_;
  std::string field_;
  friend class DBManager;

  Constraint(std::string&& cstr, std::string field = "")
      : constraint_(cstr), field_(std::move(field)) {}

 public:
  struct CompositeField {
    std::string fieldName_;
    const std::string* tableName_;
    template <typename... Args>
    CompositeField(const Args&... args) : tableName_(nullptr) {
      (Extract(args), ...);
    }

   private:
    template <typename T>
    void Extract(const tinyorm_impl::Expression::Field<T>& field) {
      if (tableName_ != nullptr && tableName_ != field.tableName_)
        throw std::runtime_error{NOT_THE_SAME_TABLE};
      tableName_ = field.tableName_;
      if (fieldName_.empty()) {
        fieldName_ = field.fieldName_;
      } else {
        fieldName_ = fieldName_ + "," + field.fieldName_;
      }
    }
  };

  template <typename T>
  static inline Constraint Default(
      const tinyorm_impl::Expression::Field<T>& field, T value) {
    std::ostringstream os;
    tinyorm_impl::Serializer::Serialize(os << " default ", value);
    return Constraint{os.str(), field.fieldName_};
  }

  static inline Constraint Check(
      const tinyorm_impl::Expression::RelationExpr& expr) {
    return Constraint{"check (" + expr.ToString() + ")"};
  }

  template <typename T>
  static inline Constraint Unique(
      const tinyorm_impl::Expression::Field<T>& field) {
    return Constraint{"unique (" + field.fieldName_ + ")"};
  }

  static inline Constraint Unique(const CompositeField& field) {
    return Constraint{"unique (" + field.fieldName_ + ")"};
  }

  template <typename T>
  static inline Constraint Reference(
      const tinyorm_impl::Expression::Field<T>& field,
      const tinyorm_impl::Expression::Field<T>& refered) {
    return Constraint{std::string("foreign key (") + field.fieldName_ +
                      ") references " + *(refered.tableName_) + "(" +
                      refered.fieldName_ + ")"};
  }

  static inline Constraint Reference(const CompositeField& fields,
                                     const CompositeField& refereds) {
    return Constraint{std::string("foreign key (") + fields.fieldName_ +
                      ") references " + *(refereds.tableName_) + "(" +
                      refereds.fieldName_ + ")"};
  }
};

template <typename Result>
class QueryResult;
}  // namespace tinyorm

namespace tinyorm_impl {

class QueryHelper {
  template <typename T>
  using Nullable = tinyorm::Nullable<T>;

  template <typename T>
  using Selectable = tinyorm_impl::Expression::FieldBase<T>;

  template <typename T>
  struct TypeToNullable {
    using type = Nullable<T>;
  };

  template <typename T>
  struct TypeToNullable<Nullable<T>> : public TypeToNullable<T> {};

  template <typename T>
  friend class tinyorm::QueryResult;

  template <typename Fn, typename TupleType>
  static inline void TupleVisit(TupleType& tuple, Fn&& fn) {
    auto func = [&fn](auto&&... args) { ((fn(args)), ...); };
    std::apply(func, tuple);
  }

  template <typename... Args>
  static inline auto FieldsToNullable(Args...) {
    return std::tuple<typename TypeToNullable<Args>::type...>{};
  }

  template <typename C>
  static inline auto QueryResultToTuple(const C& entity) {
    return tinyorm_impl::ReflectionVisitor::Visit(
        entity, [](auto... args) { return FieldsToNullable(args...); });
  }

  template <typename... Args>
  static inline auto JoinToTuple(const Args&... args) {
    return decltype(std::tuple_cat(QueryResultToTuple(args)...)){};
  }

  template <typename T>
  static inline auto SelectableToTuple(const Selectable<T>&) {
    return Nullable<T>{};
  }

  template <typename... Args>
  static inline auto SelectToTuple(const Args&... args) {
    return std::tuple<decltype(SelectableToTuple(args))...>{};
  }

  template <typename T>
  static inline std::string FieldToSql(const Selectable<T>& op) {
    if (op.tableName_)
      return (*op.tableName_) + "." + op.fieldName_;
    else
      return op.fieldName_;
  }

  template <typename T, typename... Args>
  static inline std::string FieldToSql(const Selectable<T>& arg,
                                       const Args&... args) {
    return FieldToSql(arg) + "," + FieldToSql(args...);
  }
};

}  // namespace tinyorm_impl

namespace tinyorm {

template <typename Result>
class QueryResult {
 private:
  template <typename C>
  using HasInjected = tinyorm_impl::ReflectionVisitor::HasInjected<C>;

  template <typename Q>
  friend class QueryResult;
  friend class DBManager;

  std::shared_ptr<tinyorm::DB_Base> dbhandler_;
  Result _queryHelper;
  std::string _sqlFrom;
  std::string _sqlTarget;
  std::string _sqlSelect;
  std::string _sqlWhere;
  std::string _sqlGroupBy;
  std::string _sqlHaving;
  std::string _sqlOrderBy;
  std::string _sqlLimit;
  std::string _sqlOffset;

  QueryResult(std::shared_ptr<tinyorm::DB_Base> db_ptr, Result queryHelper,
              std::string sqlFrom, std::string sqlSelect = "select ",
              std::string sqlTarget = "*", std::string sqlWhere = std::string{},
              std::string sqlGroupBy = std::string{},
              std::string sqlHaving = std::string{},
              std::string sqlOrderBy = std::string{},
              std::string sqlLimit = std::string{},
              std::string sqlOffset = std::string{})
      : dbhandler_(db_ptr),
        _queryHelper(queryHelper),
        _sqlFrom(sqlFrom),
        _sqlTarget(sqlTarget),
        _sqlSelect(sqlSelect),
        _sqlWhere(sqlWhere),
        _sqlGroupBy(sqlGroupBy),
        _sqlHaving(sqlHaving),
        _sqlOrderBy(sqlOrderBy),
        _sqlLimit(sqlLimit),
        _sqlOffset(sqlOffset) {}

  inline std::string _GetFromSql() const {
    return _sqlFrom + _sqlWhere + _sqlGroupBy + _sqlHaving;
  }

  inline std::string _GetLimit() const {
    return _sqlOrderBy + _sqlLimit + _sqlOffset;
  }

  // Select for Normal Objects
  template <typename C, typename Out>
  inline void _Select(const C&, Out& out) const {
    auto copy = _queryHelper;
    dbhandler_->ExecuteCallback(
        _sqlSelect + _sqlTarget + _GetFromSql() + _GetLimit() + ";",
        [&copy, &out](int argc, char** argv) {
          tinyorm_impl::ReflectionVisitor::Visit(copy, [argc](auto&... args) {
            if (sizeof...(args) != argc)
              throw std::runtime_error(BAD_COLUMN_COUNT);
          });
          tinyorm_impl::ReflectionVisitor::Visit(copy, [argv](auto&... args) {
            size_t idx = 0;
            ((tinyorm_impl::Deserializer::Deserialize(args, argv[idx++])), ...);
          });
          out.push_back(std::move(copy));
        });
  }

  // Select for Tuples
  template <typename Out, typename... Args>
  inline void _Select(const std::tuple<Args...>&, Out& out) const {
    auto copy = _queryHelper;
    dbhandler_->ExecuteCallback(
        _sqlSelect + _sqlTarget + _GetFromSql() + _GetLimit() + ";",
        [&copy, &out](int argc, char** argv) {
          if (sizeof...(Args) != argc)
            throw std::runtime_error(BAD_COLUMN_COUNT);
          size_t idx = 0;
          tinyorm_impl::QueryHelper::TupleVisit(copy, [argv, &idx](auto& val) {
            tinyorm_impl::Deserializer::Deserialize(val, argv[idx++]);
          });
          out.push_back(copy);
        });
  }

  template <typename... Args>
  inline QueryResult<std::tuple<Args...>> _NewQuery(
      std::string sqlTarget, std::string sqlFrom,
      std::tuple<Args...>&& newQueryHelper) const {
    return QueryResult<std::tuple<Args...>>(
        dbhandler_, newQueryHelper, std::move(sqlFrom), _sqlSelect,
        std::move(sqlTarget), _sqlWhere, _sqlGroupBy, _sqlHaving, _sqlOrderBy,
        _sqlLimit, _sqlOffset);
  }

  template <typename C>
  inline auto _NewJoinQuery(
      const C& queryHelper2,
      const tinyorm_impl::Expression::RelationExpr& onExpr,
      std::string joinStr) const {
    return _NewQuery(
        _sqlTarget,
        _sqlFrom + std::move(joinStr) +
            tinyorm_impl::ReflectionVisitor::TableName(queryHelper2) + " on " +
            onExpr.ToString(),
        tinyorm_impl::QueryHelper::JoinToTuple(_queryHelper, queryHelper2));
  }

 public:
  template <typename... Args>
  inline auto Select(const Args&... args) const {
    return _NewQuery(tinyorm_impl::QueryHelper::FieldToSql(args...), _sqlFrom,
                     tinyorm_impl::QueryHelper::SelectToTuple(args...));
  }

  inline QueryResult Distinct() const& {
    auto ret = *this;
    ret._sqlSelect = "select distinct ";
    return ret;
  }

  inline QueryResult Distinct() && {
    this->_sqlSelect = "select distinct ";
    return std::move(*this);
  }

  // Where Clause
  inline QueryResult Where(
      const tinyorm_impl::Expression::RelationExpr& expr) const& {
    auto ret = *this;
    ret._sqlWhere = " where (" + expr.ToString() + ")";
    return ret;
  }

  inline QueryResult Where(
      const tinyorm_impl::Expression::RelationExpr& expr) && {
    this->_sqlWhere = " where (" + expr.ToString() + ")";
    return std::move(*this);
  }

  // Limit Clause
  inline QueryResult Limit(size_t count) const& {
    auto ret = *this;
    ret._sqlLimit = " limit " + std::to_string(count);
    return ret;
  }

  inline QueryResult Limit(size_t count) && {
    this->_sqlLimit = " limit " + std::to_string(count);
    return std::move(*this);
  }

  // Offset Clause
  inline QueryResult Offset(size_t count) const& {
    auto ret = *this;
    if (ret._sqlLimit.empty()) ret._sqlLimit = " limit ~0";
    ret._sqlOffset = " offset " + std::to_string(count);
    return ret;
  }

  inline QueryResult Offset(size_t count) && {
    if (this->_sqlLimit.empty()) this->_sqlLimit = " limit ~0";
    this->_sqlOffset = " offset " + std::to_string(count);
    return std::move(*this);
  }

  // Group By Clause
  template <typename... Args>
  inline QueryResult GroupBy(const Args&... args) const& {
    auto ret = *this;
    ret._sqlGroupBy =
        " group by " + tinyorm_impl::QueryHelper::FieldToSql(args...);
    return ret;
  }

  template <typename... Args>
  inline QueryResult GroupBy(const Args&... args) && {
    this->_sqlGroupBy =
        " group by " + tinyorm_impl::QueryHelper::FieldToSql(args...);
    return std::move(*this);
  }

  // Having Clause
  inline QueryResult Having(
      const tinyorm_impl::Expression::RelationExpr& expr) const& {
    auto ret = *this;
    ret._sqlHaving = " having " + expr.ToString();
    return ret;
  }

  inline QueryResult Having(
      const tinyorm_impl::Expression::RelationExpr& expr) && {
    this->_sqlHaving = " having " + expr.ToString();
    return std::move(*this);
  }

  template <typename... Args>
  inline QueryResult OrderBy(const Args&... args) const& {
    auto ret = *this;
    if (ret._sqlOrderBy.empty())
      ret._sqlOrderBy =
          " order by " + tinyorm_impl::QueryHelper::FieldToSql(args...);
    else
      ret._sqlOrderBy += "," + tinyorm_impl::QueryHelper::FieldToSql(args...);
    return ret;
  }

  template <typename... Args>
  inline QueryResult OrderBy(const Args&... args) && {
    if (this->_sqlOrderBy.empty())
      this->_sqlOrderBy =
          " order by " + tinyorm_impl::QueryHelper::FieldToSql(args...);
    else
      this->_sqlOrderBy += "," + tinyorm_impl::QueryHelper::FieldToSql(args...);
    return std::move(*this);
  }

  template <typename... Args>
  inline QueryResult OrderByDescending(const Args&... args) const& {
    auto ret = std::move(this)->OrderBy(args...);
    ret._sqlOrderBy += " desc";
    return std::move(ret);
  }

  template <typename... Args>
  inline QueryResult OrderByDescending(const Args&... args) && {
    auto ret = this->OrderBy(args...);
    ret._sqlOrderBy += " desc";
    return std::move(ret);
  }

  template <typename C>
  inline auto Join(const C&, const tinyorm_impl::Expression::RelationExpr&,
                   std::enable_if_t<!HasInjected<C>::value>* = nullptr) const {}

  template <typename C>
  inline auto Join(const C& queryHelper2,
                   const tinyorm_impl::Expression::RelationExpr& onExpr,
                   std::enable_if_t<HasInjected<C>::value>* = nullptr) const {
    return _NewJoinQuery(queryHelper2, onExpr, " join ");
  }

  template <typename C>
  inline auto LeftJoin(
      const C&, const tinyorm_impl::Expression::RelationExpr&,
      std::enable_if_t<!HasInjected<C>::value>* = nullptr) const {}

  template <typename C>
  inline auto LeftJoin(
      const C& queryHelper2,
      const tinyorm_impl::Expression::RelationExpr& onExpr,
      std::enable_if_t<HasInjected<C>::value>* = nullptr) const {
    return _NewJoinQuery(queryHelper2, onExpr, " left join ");
  }

  std::vector<Result> ToVector() const {
    std::vector<Result> ret;
    _Select(_queryHelper, ret);
    return ret;
  }
};

/**
 * @todo
 */
class DBManager {
 private:
  template <typename C>
  using HasInjected = tinyorm_impl::ReflectionVisitor::HasInjected<C>;
  std::shared_ptr<DB_Base> dbhandler_;

  template <typename... Args>
  static void _GetConstraint(
      std::string& tableFixes,
      std::unordered_map<std::string, std::string>& fieldFixes,
      const Args&... args) {
    auto GetConstraintHelper = [&tableFixes,
                                &fieldFixes](const Constraint& cstr) {
      if (!cstr.field_.empty()) {
        fieldFixes[cstr.field_] += cstr.constraint_;
      } else {
        tableFixes += (cstr.constraint_ + ",");
      }
    };
    (GetConstraintHelper(args), ...);
  }

  template <typename C>
  static inline void _GetInsert(std::ostream& os, const C& entity,
                                bool withPrimaryKey) {
    tinyorm_impl::ReflectionVisitor::Visit(
        entity, [&os, &entity, withPrimaryKey](const auto& primaryKey,
                                               const auto&... args) {
          const auto& fieldNames =
              tinyorm_impl::ReflectionVisitor::FieldNames(entity);
          os << "insert into "
             << tinyorm_impl::ReflectionVisitor::TableName(entity) << "(";
          bool extra_comma = false;
          std::ostringstream osVal;  //!< Serialize field values.
          auto serializeField = [&fieldNames, &os, &osVal, &extra_comma](
                                    const auto& val, size_t index) {
            if (tinyorm_impl::Serializer::Serialize(osVal, val)) {
              os << fieldNames[index] << ",";
              osVal << ",";
              extra_comma = true;
            }
          };
          // insert the primary key
          if (withPrimaryKey &&
              tinyorm_impl::Serializer::Serialize(osVal, primaryKey)) {
            os << fieldNames[0] << ",";
            osVal << ",";
            extra_comma = true;
          }

          // insert the rest
          size_t index = 1;
          (serializeField(args, index++), ...);

          if (extra_comma) {
            os.seekp(os.tellp() - std::streamoff(1));
            osVal.seekp(osVal.tellp() - std::streamoff(1));
          } else {
            os << fieldNames[0];
            osVal << "null";
          }
          osVal << ");";
          os << ") values (" << osVal.str();
        });
  }

  template <typename C>
  static inline bool _GetUpdate(std::ostream& os, const C& entity) {
    return tinyorm_impl::ReflectionVisitor::Visit(
        entity, [&os, &entity](const auto& primaryKey, const auto&... args) {
          if (sizeof...(args) == 0) return false;
          const auto& fieldNames =
              tinyorm_impl::ReflectionVisitor::FieldNames(entity);
          const auto& tableName =
              tinyorm_impl::ReflectionVisitor::TableName(entity);
          os << "update " << tableName << " set ";

          auto serializeField = [&fieldNames, &os](const auto& val,
                                                   size_t index) {
            os << fieldNames[index] << "=";
            if (!tinyorm_impl::Serializer::Serialize(os, val)) os << "null";
            os << ",";
          };

          size_t idx = 1;
          (serializeField(args, idx++), ...);
          os.seekp(os.tellp() - std::streamoff(1));

          os << " where " << tableName << "." << fieldNames[0] << "=";
          if (!tinyorm_impl::Serializer::Serialize(os, primaryKey))
            os << "null";
          os << ";";
          return true;
        });
  }

 public:
  DBManager(std::shared_ptr<DB_Base>&& db) : dbhandler_(std::move(db)) {}
  ~DBManager() = default;

  template <typename C, typename... Args>
  std::enable_if_t<!HasInjected<C>::value> CreateTbl(const C&, const Args&...) {
  }

  template <typename C, typename... Args>
  std::enable_if_t<HasInjected<C>::value> CreateTbl(const C& entity,
                                                    const Args&... cstrs) {
    const auto& fieldNames =
        tinyorm_impl::ReflectionVisitor::FieldNames(entity);
    std::unordered_map<std::string, std::string> fieldFixes;
    [[maybe_unused]] auto addTypeStr = [&fieldNames, &fieldFixes](
                                           const auto& arg, size_t idx) {
      constexpr const char* typeStr =
          tinyorm_impl::TypeString<std::decay_t<decltype(arg)>>::type_string;
      fieldFixes.emplace(fieldNames[idx], typeStr);
    };
    tinyorm_impl::ReflectionVisitor::Visit(entity,
                                           [&addTypeStr](const auto&... args) {
                                             size_t idx = 0;
                                             (addTypeStr(args, idx++), ...);
                                           });

    fieldFixes[fieldNames[0]] += " primary key ";
    std::string tableFixes;
    _GetConstraint(tableFixes, fieldFixes, cstrs...);

    std::string strFmt;
    for (const auto& field : fieldNames) {
      strFmt += (field + fieldFixes[field] + ",");
    }
    strFmt += std::move(tableFixes);
    strFmt.pop_back();

    dbhandler_->Execute("create table " +
                        tinyorm_impl::ReflectionVisitor::TableName(entity) +
                        "(" + strFmt + ");");
  }

  template <typename C>
  std::enable_if_t<!HasInjected<C>::value> DropTbl(const C&) {}

  template <typename C>
  std::enable_if_t<HasInjected<C>::value> DropTbl(const C& entity) {
    dbhandler_->Execute("drop table " +
                        tinyorm_impl::ReflectionVisitor::TableName(entity) +
                        ";");
  }

  template <typename C>
  std::enable_if_t<!HasInjected<C>::value> Delete(const C&) {}

  /**
   * @brief Delete record according to the primary key.
   */
  template <typename C>
  std::enable_if_t<HasInjected<C>::value> Delete(const C& entity) {
    const auto& fieldNames =
        tinyorm_impl::ReflectionVisitor::FieldNames(entity);
    std::ostringstream os;
    os << "delete from " << tinyorm_impl::ReflectionVisitor::TableName(entity)
       << " where " << fieldNames[0] << "=";
    tinyorm_impl::ReflectionVisitor::Visit(
        entity, [&os](const auto& primaryKey, const auto&... dummy) {
          if (!tinyorm_impl::Serializer::Serialize(os, primaryKey)) {
            os << "null";
          }
        });
    os << ";";
    dbhandler_->Execute(os.str());
  }

  template <typename C>
  std::enable_if_t<!HasInjected<C>::value> Delete(
      const C&, const tinyorm_impl::Expression::RelationExpr&) {}

  template <typename C>
  std::enable_if_t<HasInjected<C>::value> Delete(
      const C& entity, const tinyorm_impl::Expression::RelationExpr& expr) {
    dbhandler_->Execute("delete from " +
                        tinyorm_impl::ReflectionVisitor::TableName(entity) +
                        " where " + expr.ToString() + ";");
  }

  template <typename C>
  std::enable_if_t<!HasInjected<C>::value> Insert(const C&, bool = true) {}

  template <typename C>
  std::enable_if_t<HasInjected<C>::value> Insert(const C& entity,
                                                 bool withPrimaryKey = true) {
    std::ostringstream os;
    _GetInsert(os, entity, withPrimaryKey);
    dbhandler_->Execute(os.str());
  }

  template <typename In, typename C = typename In::value_type>
  std::enable_if_t<!HasInjected<C>::value> InsertRange(const In&, bool);

  template <typename In, typename C = typename In::value_type>
  std::enable_if_t<HasInjected<C>::value> InsertRange(
      const In& entities, bool withPrimaryKey = true) {
    std::ostringstream os;
    if (!entities.empty()) {
      for (const auto& entity : entities) {
        _GetInsert(os, entity, withPrimaryKey);
      }
      dbhandler_->Execute(os.str());
    }
  }

  template <typename C>
  std::enable_if_t<!HasInjected<C>::value> Update(const C&) {}

  template <typename C>
  std::enable_if_t<HasInjected<C>::value> Update(const C& entity) {
    std::ostringstream os;
    if (_GetUpdate(os, entity)) dbhandler_->Execute(os.str());
  }

  template <typename C>
  std::enable_if_t<!HasInjected<C>::value> Update(
      const C&, const tinyorm_impl::Expression::AssignmentExpr&,
      const tinyorm_impl::Expression::RelationExpr&) {}

  template <typename C>
  std::enable_if_t<HasInjected<C>::value> Update(
      const C& entity,
      const tinyorm_impl::Expression::AssignmentExpr& assignClause,
      const tinyorm_impl::Expression::RelationExpr& whereClause) {
    dbhandler_->Execute("update " +
                        tinyorm_impl::ReflectionVisitor::TableName(entity) +
                        " set " + assignClause.ToString() + " where " +
                        whereClause.ToString() + ";");
  }

  template <typename In, typename C = typename In::value_type>
  std::enable_if_t<!HasInjected<C>::value> UpdateRange(const In&) {}

  template <typename In, typename C = typename In::value_type>
  std::enable_if_t<HasInjected<C>::value> UpdateRange(const In& entities) {
    std::ostringstream os, osTmp;
    for (const auto& entity : entities) {
      if (_GetUpdate(osTmp, entity)) {
        os << osTmp.str();
        osTmp.str("");
      }
    }
    auto sql = os.str();
    if (!sql.empty()) dbhandler_->Execute(sql);
  }

  template <typename C>
  std::enable_if_t<!HasInjected<C>::value, QueryResult<C>> Query(const C&) {}

  template <typename C>
  std::enable_if_t<HasInjected<C>::value, QueryResult<C>> Query(C queryHelper) {
    return QueryResult<C>(
        dbhandler_, std::move(queryHelper),
        std::string(" from ") +
            tinyorm_impl::ReflectionVisitor::TableName(queryHelper));
  }
};

}  // namespace tinyorm

#undef NO_SUCH_FIELD
#undef NO_REFLECTIONED
#undef BAD_TYPE
#undef NOT_THE_SAME_TABLE
#undef BAD_COLUMN_COUNT
#endif  // TINYORM_H_
