#pragma once

#include<memory>
#include<string>
#include<type_traits>
#include<vector>
#include<sys/uio.h>
#include"util.h"
#include"macro.h"
#include<string.h>
/******
 * 序列化---反序列化
 * 大小端转换---直接写---write_help  整数
 * 编码映射---write_with_zigzag  int64_t int32_t
 * 浮点数---转换为整数---用第一种写
 * **/

namespace HSQ
{
static auto logger=GET_LOGGER("system");
class ByteArray
{
public:
    using ptr=std::shared_ptr<ByteArray>;
    ByteArray(size_t size=4096);
    ~ByteArray();

    //写入数据
    template<typename T>
    void Write(T val,bool use_zigzag=false);

    //写入字符串
    void Write(const std::string& val,bool str=true);

    //读取数据
    template<typename T>
    void Read(T& val,bool use_zigzag=false);

     //读取字符串
    void Read( std::string& val,bool str=true);

    //查看和设置字节序
    bool IsLittelEndian()const;
    void SetIsLittleEndian(bool flag);


    void Clear();


    void read_help(void* buf, size_t size, size_t position) const;

    /**
     * @brief 返回ByteArray当前位置
     */
    size_t GetPosition() const { return cur_pos;}


    void SetPosition(size_t v);

    //把ByteArray的数据写入到文件中
    bool WriteToFile(const std::string& name) const;


    bool ReadFromFile(const std::string& name);


    size_t GetBaseSize() const { return default_size;}

    size_t GetReadSize() const { return cur_used ;}


    std::string ToString() const;


    std::string ToHexString() const;


    uint64_t GetReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;


    uint64_t GetReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;


    uint64_t GetWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    size_t GetSize() const { return default_size;}

    //静态成员函数
    static uint32_t EncodeZigzag32(const int32_t& v);
    static uint64_t EncodeZigzag64(const int64_t& v) ;
    static int32_t  DecodeZigzag32(const uint32_t& v);
    static int64_t DecodeZigzag64(const uint64_t& v); 
private:
    struct Node
    {
        Node(size_t sz);
        Node();
        ~Node();
        char* ptr;
        Node* next_node;
        size_t node_size;
    };
private:
    //内存块大小
    size_t default_size;
    //当前在内存块中的位置,内存地址
    size_t cur_pos;
    //当前容量
    size_t cur_capacity;
    //当前已经使用的大小
    size_t cur_used;

    int8_t data_endian;
    //第一个内存块指针
    Node* first_node;
    //当前内存块指针
    Node* cur_node;

private:
    //help函数实际写入内存
    void write_help(const void* buf,size_t size);
    void read_help(void* buf,size_t size);

    template<typename T>
    void write_with_zigzag(T val);

    template<typename T>
    void read_with_zigzag(T& val);

    void AddCapacity(size_t size);

    size_t GetCapacity()const
    {
        return cur_capacity-cur_pos;
    }


};

template<typename T>
void ByteArray::Write(T val,bool use_zigzag)
{
    //浮点型：float---uint32_t  double---uint64_t
    if(std::is_floating_point<T>::value)       
    {
        using data_type=typename std::conditional<std::is_same<T,float>::value,uint32_t,uint64_t>::type;
        data_type value;
        memcpy(&value,&val,sizeof(value));
        //转换后，用写整数的方式直接写，大小端转换，不压缩
        Write<data_type>(value,false);
    }
    //写入整形：大小端转换---write_help，编码映射---write_with_zigzag
    else if(std::is_arithmetic<T>::value)
    {
        if(!use_zigzag)
        {
            //DEBUG(logger,std::to_string(sizeof(T)));
            if(data_endian!=HSQ_BYTE_ORDER && (!std::is_same<T,uint8_t>::value))
                val=byteswap(val);
            write_help(&val,sizeof(val));
        }
        else if(std::is_same<T,int32_t>::value || std::is_same<T,int64_t>::value)
        {
            //编码处理只针对32未和64位有符号
            if(std::is_same<T,int32_t>::value)
                write_with_zigzag<uint32_t>(ByteArray::EncodeZigzag32(val));
            else
                write_with_zigzag<uint64_t>(ByteArray::EncodeZigzag64(val));
        }
        else
        {
            std::stringstream ss;
            ss<<"写入错误：T="<<typeid(val).name()<<"  val="<<val;
            ERROR(logger,ss.str());
        }
           
    }
}

//写入编码后的数据
template<typename T>
void ByteArray::write_with_zigzag(T val)
{
    if(std::is_same<T,uint32_t>::value)
    {
        uint8_t tmp[5];
        uint8_t i = 0;
        while(val >= 0x80) 
        {
            tmp[i++] = (val & 0x7F) | 0x80;
            val >>= 7;
        }
        tmp[i++] = val;
        write_help(tmp, i);
    }
    else if(std::is_same<T,uint64_t>::value)
    {
        uint8_t tmp[10];
        uint8_t i = 0;
        while(val >= 0x80) 
        {
            tmp[i++] = (val & 0x7F) | 0x80;
            val >>= 7;
        }
        tmp[i++] = val;
        write_help(tmp, i);
    }
    else
    {
        std::stringstream ss;
        ss<<"write_with_zigzag 写入错误：T="<<typeid(val).name()<<"  val="<<val;
        ERROR(logger,ss.str());
    }
}


template<typename T>
void ByteArray::Read(T& val,bool use_zigzag)
{
    if(std::is_floating_point<typename std::decay<T>::type>::value)
    {
        using data_type=typename std::conditional<std::is_same<typename std::decay<T>::type,float>::value,uint32_t,uint64_t>::type;
        data_type data;
        read_help(&data,sizeof(val));
        if(data_endian!=HSQ_BYTE_ORDER)
            data=byteswap(data);
        memcpy(&val,&data,sizeof(data));
    }
    //整形：是否压缩
    else if(std::is_arithmetic<T>::value)
    {
        typename std::decay<T>::type data;
        //有压缩
        if(!use_zigzag)
        {
            read_help(&data,sizeof(data));
            if(data_endian!=HSQ_BYTE_ORDER && sizeof(T)>1)
                data=byteswap(data);
            memcpy(&val,&data,sizeof(data));
        }
        else if(std::is_same<typename std::decay<T>::type,int32_t>::value || std::is_same<typename std::decay<T>::type,int64_t>::value)
        {
           if(std::is_same<typename std::decay<T>::type,uint32_t>::value)
           {
                uint32_t data;
                read_with_zigzag<uint32_t>(data);
                val=ByteArray::DecodeZigzag32(data);
           }
           else
           {
                uint64_t data;
                read_with_zigzag<uint64_t>(data);
                val=ByteArray::DecodeZigzag64(data);
           }
        }
        else
        {
            std::stringstream ss;
            ss<<"读取错误：T="<<typeid(val).name();
            ERROR(logger,ss.str());
        }
    }
}

template<typename T>
void ByteArray::read_with_zigzag(T& val)
{
    if(std::is_same<typename std::decay<T>::type,uint32_t>::value)
    {
        uint32_t result = 0;
        for(int i = 0; i < 32; i += 7)
        {
            uint8_t b ;
            read_help(&b,1);
            if(b < 0x80)
            {
                result |= ((uint32_t)b) << i;
                break;
            }
            else
                result |= (((uint32_t)(b & 0x7f)) << i);
        }
        val=result;
    }
    if(std::is_same<typename std::decay<T>::type,uint64_t>::value)
    {
        uint64_t result = 0;
        for(int i = 0; i < 64; i += 7) 
        {
            uint8_t b ;
            read_help(&b,1);
            if(b < 0x80) 
            {
                result |= ((uint64_t)b) << i;
                break;
            } 
            else 
                result |= (((uint64_t)(b & 0x7f)) << i);     
        }
        val=result;
    }
    else
    {
        std::stringstream ss;
        ss<<"read_with_zigzag 读取错误：T="<<typeid(val).name();
        ERROR(logger,ss.str());
    }
}


}