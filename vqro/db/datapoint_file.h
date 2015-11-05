#ifndef VQRO_DB_DATAPOINT_FILE_H
#define VQRO_DB_DATAPOINT_FILE_H

#include <cstdint>
#include <gflags/gflags.h>
#include "vqro/db/datapoint.h"
#include "vqro/db/read_op.h"
#include "vqro/db/write_op.h"


DECLARE_int32(datapoint_file_mode);


namespace vqro {
namespace db {


class DatapointDirectory;


class DatapointFile {
 public:
  DatapointDirectory* const dir;
  int64_t min_timestamp;
  int64_t max_timestamp;

  DatapointFile() = default;
  DatapointFile(DatapointDirectory* _dir, int64_t _min, int64_t _max) :
    dir(_dir),
    min_timestamp(_min),
    max_timestamp(_max) {}

  virtual ~DatapointFile() {}
  virtual string GetPath() const = 0;
  virtual void Read(ReadOperation& read_op) const = 0;
  virtual size_t Write(const WriteOperation& write_op) = 0;
  virtual size_t RemainingWritableDatapoints() const = 0;

  bool operator<(const DatapointFile& rhs) const {
    return min_timestamp < rhs.min_timestamp;
  }
};


} // namespace db
} // namespace vqro

#endif // VQRO_DB_DATAPOINT_FILE_H
