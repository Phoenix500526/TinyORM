#ifndef TINYORM_DB_BASE_H_
#define TINYORM_DB_BASE_H_

#include <functional>

namespace tinyorm {

class DB_Base {
 public:
  virtual void ForeignKeyOn() = 0;
  virtual void Execute(const std::string&) = 0;
  virtual void ExecuteCallback(const std::string&,
                               std::function<void(int, char**)>) = 0;
  virtual ~DB_Base() {}
};

/**
记录一个想法：
1. 像 STL 那样，设置相应的数据库类别：database_catagory{mysql_type,
sqlite3_type}
2. 实现一个 dispatch 对象，他会接受 database_catagory 对象，然后根据 *_type
进行派发
 */

class DB_Sqlite : public DB_Base {
 public:
  DB_Sqlite(const std::string& conn) {}
  ~DB_Sqlite() {}
  void ForeignKeyOn() override { std::cout << "ForeignKeyOn" << std::endl; }
  void Execute(const std::string& cmd) override {
    std::cout << cmd << std::endl;
  }
  void ExecuteCallback(const std::string& cmd,
                       std::function<void(int, char**)> callback) {
    std::cout << cmd << std::endl;
  }
};

}  // namespace tinyorm
#endif  // TINYORM_DB_BASE_H_