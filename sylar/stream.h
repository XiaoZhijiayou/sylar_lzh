#ifndef __SYLAR_STREAM_H__
#define __SYLAR_STREAM_H__

#include <memory>
#include "bytearray.h"

namespace sylar {

/**
 * @brief 流结构
 */
class Stream {
 public:
  typedef std::shared_ptr<Stream> ptr;

  /**
   * @brief 析构函数
   */
  virtual ~Stream() {}

  /**
   * @brief 读数据
   * @param buffer 接收数据的内存
   * @param length 接收数据的内存大小
   * @return
   *    @retval > 0 返回接收到的数据的实际大小
   *    @retval = 0 被关闭
   *    @retval < 0 出现流错误
   */
  virtual int read(void* buffer, size_t length) = 0;

  /**
   * @brief 读数据
   * @param ba 接收数据的ByteArray
   * @param length 接收数据的内存大小
   * @return
   *    @retval > 0 返回接收到的数据的实际大小
   *    @retval = 0 被关闭
   *    @retval < 0 出现流错误
   */
  virtual int read(ByteArray::ptr ba, size_t length) = 0;

  /**
   * @brief 读固定长度的数据
   * @param buffer 接收数据的内存
   * @param length 接收数据的内存大小
   * @return
   *    @retval > 0 返回接收到的数据的实际大小
   *    @retval = 0 被关闭
   *    @retval < 0 出现流错误
   */
  virtual int readFixSize(void* buffer, size_t length);

  /**
   * @brief 读固定长度的ByteArray
   * @param buffer 接收数据的ByteArray
   * @param length 接收数据的内存大小
   * @return
   *    @retval > 0 返回接收到的数据的实际大小
   *    @retval = 0 被关闭
   *    @retval < 0 出现流错误
   */
  virtual int readFixSize(ByteArray::ptr ba, size_t length);

  /**
   * @brief 写数据
   * @param buffer 写数据的内存
   * @param length 写数据的内存大小
   * @return
   *    @retval > 0 返回写入的数据的实际大小
   *    @retval = 0 被关闭
   *    @retval < 0 出现流错误
   */
  virtual int write(const void* buffer, size_t length) = 0;

  /**
   * @brief 写数据
   * @param buffer 写数据的ByteArray
   * @param length 写数据的内存大小
   * @return
   *    @retval > 0 返回写入的数据的实际大小
   *    @retval = 0 被关闭
   *    @retval < 0 出现流错误
   */
  virtual int write(ByteArray::ptr ba, size_t length) = 0;

  /**
   * @brief 写固定长度的数据
   * @param buffer 写数据的内存
   * @param length 写数据的内存大小
   * @return
   *    @retval > 0 返回接收到的数据的实际大小
   *    @retval = 0 被关闭
   *    @retval < 0 出现流错误
   */
  virtual int writeFixSize(const void* buffer, size_t length);

  /**
   * @brief 写固定长度的数据
   * @param buffer 写数据的内存
   * @param length 写数据的内存大小
   * @return
   *    @retval > 0 返回接收到的数据的实际大小
   *    @retval = 0 被关闭
   *    @retval < 0 出现流错误
   */
  virtual int writeFixSize(ByteArray::ptr ba, size_t length);

  /**
   * @brief 关闭流
   */
  virtual void close() = 0;
};

}  // namespace sylar

#endif