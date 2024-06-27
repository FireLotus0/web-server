#include"bytearray.h"


#include<sstream>
#include<math.h>
#include<typeinfo>

namespace HSQ
{

//字符串
//读取字符串
void ByteArray::Read( std::string& val,bool str)
{
    uint64_t len{0};
    Read<uint64_t>(len,false);
    val.resize(len);
    read_help(&val[0],len);
}

//写入字符串
void ByteArray::Write(const std::string& val,bool str)
{
    Write<uint64_t>((uint64_t)val.size(),false);
    DEBUG(logger,"in Write len="+std::to_string((uint64_t)val.size()));
    write_help(val.c_str(),size_t(val.size()));
}


//zigzag编码解码
uint32_t ByteArray::EncodeZigzag32(const int32_t& v) {
    if(v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

uint64_t ByteArray::EncodeZigzag64(const int64_t& v) {
    if(v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

int32_t ByteArray::DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

int64_t ByteArray::DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}



//ByteArray构造函数
ByteArray::ByteArray(size_t size)
{
    default_size=size;
    cur_pos=0;
    cur_capacity=size;
    cur_used=0;
    cur_node=new Node(size);
    first_node=cur_node;
    data_endian=HSQ_BIG_ENDIAN;
}
//ByteArray析构函数
ByteArray::~ByteArray()
{
    Node* tmp=first_node;
    while(tmp!=nullptr)
    {
        cur_node=tmp->next_node;
        delete tmp;
        tmp=cur_node;
    }
}
//Node构造函数
ByteArray::Node::Node()
{
    ptr=nullptr;
    next_node=nullptr;
    node_size=0;
}
ByteArray::Node::Node(size_t sz)
{
    ptr=new char[sz];
    next_node=nullptr;
    node_size=sz;
}
ByteArray::Node::~Node()
{
    if(ptr)
        delete []ptr;
}

//查看和设置字节序
bool ByteArray::IsLittelEndian()const
{
    return data_endian==HSQ_LITTLE_ENDIAN;
}
void ByteArray::SetIsLittleEndian(bool flag)
{
    if(flag)
        data_endian=HSQ_LITTLE_ENDIAN;
    else
        data_endian=HSQ_BIG_ENDIAN;
}

void ByteArray::Clear()
{
    cur_pos=cur_used=0;
    cur_capacity=default_size;
    Node* temp=first_node->next_node;
    while(temp)
    {
        cur_node=temp->next_node;
        delete temp;
        temp=cur_node;
    }
    cur_node=first_node;
    first_node->next_node=nullptr;
}


 //help函数实际写入内存
void ByteArray::write_help(const void* buf,size_t size)
{
    if(size==0)
        return;
   
    AddCapacity(size);
    //当前内存块中的位置对内存块大小取模得到偏移量
    size_t now_pos=cur_pos %default_size;
    //当前内存块的大小减去当前在内存块中的位置=当前内存块的可用容量
    
    size_t now_capacity=cur_node->node_size-now_pos;

    size_t buf_pos=0;
    while(size>0)
    {
        //当前容量写的下
        if(now_capacity>=size)
        {
            //写入数据
            memcpy(cur_node->ptr+now_pos,(const char*)buf+buf_pos,size);
            //如果当前内存块写满了，更新node指针
            if(cur_node->node_size==(now_pos+size))
                cur_node=cur_node->next_node;
            buf_pos+=size;
            cur_pos+=size;
            size=0;
            
        }
        //当前容量不足
        else
        {
            memcpy(cur_node->ptr+now_pos,(const char*)buf+buf_pos,now_capacity);
            cur_pos+=now_capacity;
            buf_pos+=now_capacity;
            size-=now_capacity;
            cur_node=cur_node->next_node;
            now_capacity=cur_node->node_size;
            now_pos=0;
        }
    }
    if(cur_pos > cur_used)
        cur_used=cur_pos;
   
}
void ByteArray::read_help(void* buf,size_t size)
{
    if(size>GetReadSize())
    {
        throw std::out_of_range("size>GetReadSize()");
    }
    //当前内存块中的位置对内存块大小取模得到偏移量
    size_t now_pos=cur_pos %default_size;
    //当前内存块的大小减去当前在内存块中的位置=当前内存块的可用容量
    size_t now_capacity=cur_node->node_size-now_pos;

    size_t buf_pos=0;

    while(size>0)
    {
        //当前容量写的下
        if(now_capacity>=size)
        {
            //写入数据
            memcpy((char*)buf+buf_pos,cur_node->ptr+now_pos,size);
            //如果当前内存块写满了，更新node指针
            if(cur_node->node_size==(now_pos+size))
                cur_node=cur_node->next_node;
            cur_pos+=size;
            buf_pos+=size;
            size=0;
        }
        //当前容量不足
        else
        {
            memcpy((char*)buf+buf_pos,cur_node->ptr+now_pos,now_capacity);
            cur_pos+=now_capacity;
            buf_pos+=now_capacity;
            size-=now_capacity;
            cur_node=cur_node->next_node;
            now_capacity=cur_node->node_size;
            now_pos=0;
        }
    }
}

void ByteArray::read_help(void* buf, size_t size, size_t position) const
{
     if(size>default_size-position)
    {
        throw std::out_of_range("size>default_size-position");
    }
    //当前内存块中的位置对内存块大小取模得到偏移量
    size_t now_pos=position%default_size;
    //当前内存块的大小减去当前在内存块中的位置=当前内存块的可用容量
    size_t now_capacity=cur_node->node_size-now_pos;

    size_t buf_pos=0;
    Node* cur=cur_node;
    while(size>0)
    {
        //当前容量写的下
        if(now_capacity>=size)
        {
            //写入数据
            memcpy((char*)buf+buf_pos,cur_node->ptr+now_pos,size);
            //如果当前内存块写满了，更新node指针
            if(cur->node_size==(now_pos+size))
                cur=cur->next_node;
            position+=size;
            buf_pos+=size;
            size=0;
        }
        //当前容量不足
        else
        {
            memcpy((char*)buf+buf_pos,cur->ptr+now_pos,now_capacity);
            position+=now_capacity;
            buf_pos+=now_capacity;
            size-=now_capacity;
            cur=cur->next_node;
            now_capacity=cur->node_size;
            now_pos=0;
        }
    }
}


void ByteArray::SetPosition(size_t v)
{
    if(v>cur_capacity)
        throw std::out_of_range("set_position out of range");
    
    cur_pos=v;
    if(cur_pos > default_size)
        default_size=cur_pos;

    cur_node=first_node;
    while(v>cur_node->node_size)
    {
        v-=cur_node->node_size;
        cur_node=cur_node->next_node;
    }
    if(v==cur_node->node_size)
        cur_node=cur_node->next_node;
}


//向文件写入数据
bool ByteArray::WriteToFile(const std::string& name) const 
{
    std::ofstream os;
    os.open(name,std::ios::trunc | std::ios::binary);
    if(!os.is_open())
    {
        std::stringstream ss;
        ss<< "writeToFile name=" << name << " error , errno=" << errno << " errstr=" << strerror(errno);
        ERROR(logger,ss.str());
        return false;
    }

    int64_t read_size=GetReadSize();
    int64_t pos=cur_pos;
    Node* cur=cur_node;

    while(read_size>0)
    {
        int distance=pos%default_size;
        int64_t len=(read_size>(int64_t)default_size?default_size:read_size)-distance;
    
        os.write(cur->ptr+distance,len);
        cur=cur->next_node;
        pos+=len;
        read_size-=len;

    }
    return true;
}

bool ByteArray::ReadFromFile(const std::string& name)
{
    std::ifstream os;
    os.open(name, std::ios::binary);
    if(!os.is_open())
    {
        std::stringstream ss;
        ss<< "ReadFromFile name=" << name << " error , errno=" << errno << " errstr=" << strerror(errno);
        ERROR(logger,ss.str());
        return false;
    }

    std::shared_ptr<char> buf(new char[default_size],[](char* p){delete []p;});
    while(!os.eof())
    {
        os.read(buf.get(),default_size);
        write_help(buf.get(),os.gcount());
    }
    return true;
}


void ByteArray::AddCapacity(size_t size)
{
    if(size==0)
        return;
    size_t old_capacity=GetCapacity();
    if(old_capacity>=size)
        return;
    
    size-=old_capacity;
    size_t count=ceil(1.0*size / default_size);
    Node* temp=first_node;
    while(temp->next_node)
        temp=temp->next_node;
    
    Node* first=nullptr;
    for(size_t i=0;i<count;++i)
    {
        temp->next_node=new Node(default_size);
        if(first==nullptr)
            first=temp->next_node;
        temp=temp->next_node;
        cur_capacity+=default_size;
    }
    if(old_capacity==0)
        cur_node=first;
}


std::string ByteArray::ToString() const
{
    std::string str;
    str.resize(GetBaseSize());
    if(str.empty())
        return str;
    
    read_help(&str[0],str.size(),cur_pos);
    return str;
}

std::string ByteArray::ToHexString() const 
{
    std::string str = ToString();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}

uint64_t ByteArray::GetReadBuffers(std::vector<iovec>& buffers, uint64_t len) const
{
    len=len>GetReadSize() ?GetReadSize():len;

    if(len==0)
        return 0;
    
    uint64_t size = len;

    size_t npos = cur_pos % default_size;
    size_t ncap = cur_node->node_size - npos;
    struct iovec iov;
    Node* cur = cur_node;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next_node;
            ncap = cur->node_size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;

}

uint64_t ByteArray::GetReadBuffers(std::vector<iovec>& buffers ,uint64_t len, uint64_t position) const
{
     len = len > GetReadSize() ? GetReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = position % default_size;
    size_t count = position / default_size;
    Node* cur = first_node;
    while(count > 0) {
        cur = cur->next_node;
        --count;
    }

    size_t ncap = cur->node_size - npos;
    struct iovec iov;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next_node;
            ncap = cur->node_size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::GetWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    if(len == 0) {
        return 0;
    }
    AddCapacity(len);
    uint64_t size = len;

    size_t npos = cur_pos% default_size;
    size_t ncap = cur_node->node_size - npos;
    struct iovec iov;
    Node* cur = cur_node;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next_node;
            ncap = cur->node_size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

}