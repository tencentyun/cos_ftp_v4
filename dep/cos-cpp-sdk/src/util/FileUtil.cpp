#include "util/FileUtil.h"
#include "util/CodecUtil.h"
#include "util/StringUtil.h"
#include "CosSysConfig.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "CosDefines.h"

using namespace std;
namespace qcloud_cos {
    
string FileUtil::getFileContent(const string &localFilePath) 
{   
    std::ifstream fileInput(localFilePath.c_str(), std::ios::in | std::ios::binary);
    std::ostringstream out;

    out << fileInput.rdbuf();
    string content = out.str();

    fileInput.close();
    fileInput.clear();

    return content;
}

uint64_t FileUtil::getFileLen(const string &localFilePath)
{
    std::ifstream fileInput(localFilePath.c_str(), std::ios::in | std::ios::binary);
    fileInput.seekg(0, std::ios::end);
    uint64_t fileLen = fileInput.tellg();
    fileInput.close();
    return fileLen;
}

bool FileUtil::isFileExists(const string &filename)
{
    ifstream fin(filename.c_str() , std::ios::in);
    if (!fin)
    {
        return false;
    }
    fin.close();
    return true;
}

string FileUtil::FormatFolderPath(const string &path) {
    string folderPathStr = path;
    size_t len = folderPathStr.length();
    if (len > 0 && folderPathStr[len - 1] != kPathDelimiterChar) {
        folderPathStr.append(kPathDelimiter);
    }
    return FormatPath(folderPathStr);
}

string FileUtil::FormatFilePath(const string &path) {
    string filePathStr = path;
    size_t len = filePathStr.length();
    if (len > 0 && filePathStr[len - 1] == kPathDelimiterChar) {
        filePathStr.erase(len - 1, 1);
    }
    return FormatPath(filePathStr);
}

bool FileUtil::IsLegalFilePath(const string &path) {
    size_t len = path.length();
    if (len == 0 || path[0] != kPathDelimiterChar || path[len -1] == kPathDelimiterChar) {
        return false;
    } else {
        return true;
    }
}

bool FileUtil::isValidFolderName(const string& foldername)
{
    if (foldername.length() == 0)
    {
        return false;
    }

    //判断全空的情况
    bool all_scape_flag = true;
    for(unsigned int i=0; i < foldername.length(); i++)
    {
        if (foldername.at(i) != ' ')
        {
            all_scape_flag = false;
            break;
        }
    }

    if (all_scape_flag == true)
    {
        return false;
    }
    
    //判断名称中是否包含保留字符'/' , '?' , '*' , ':' , '|' , '\' , '<' , '>' , '"'
    static const char reserve_char[] = {'/' , '?' , '*' , ':' , '|' , '\\' , '<' , '>' , '\"','\0'};
    static const int reserve_char_len = sizeof(reserve_char) / sizeof(reserve_char[0]) - 1;
    for(int i=0; i < reserve_char_len; i++)
    {
        if (string::npos != foldername.find(reserve_char[i], 0))
        {
            return false;
        }
    }

    return true;
}

bool FileUtil::isLegalFolderPath(const string &path) {
    size_t len = path.length();
    //文件夹前后都要带'/'
    if (len == 0 || path[0] != kPathDelimiterChar || path[len -1] != kPathDelimiterChar) 
    {
        return false;
    } 
    else 
    {
        unsigned int index = 0;
        unsigned int index2 = 0;
        string foldername;
        while (true)
        {
            /* dstpath= /folder1/folder2/folder3/  */
            if (index >= path.length() - 1)
            {
                break;
            }
            index2 = path.find_first_of("/",index+1);
            if (index2 == string::npos)
            {
                break;
            }
            
            //连续两个'/'的情况
            if (index2 == index +1)
            {
                return false;
            }

            foldername = path.substr(index+1, index2 - index - 1);
            //判断文件夹是否包含保留字符
            if (!isValidFolderName(foldername)){
                 return false; 
            }

            index = index2;
        }

        return true;
    }
}

bool FileUtil::IsRootPath(const string &dstPath) {
    return (dstPath == kPathDelimiter);
}

string FileUtil::EncodePath(const string &dstPath) {
    string encodeStr = "";
    if (dstPath.empty()) {
        return encodeStr;
    }
    if (dstPath.find(kPathDelimiter) != 0) {
        encodeStr.append(kPathDelimiter);
    }
    size_t total_len = dstPath.length();
    size_t pos = 0;
    size_t next_pos = 0;
    string tmp_str;
    while (pos < total_len) {
        next_pos = dstPath.find(kPathDelimiter, pos);
        if (next_pos == string::npos) {
            next_pos = total_len;
        }
        tmp_str = dstPath.substr(pos, next_pos - pos);
        //StringUtil::Trim(tmp_str); //会导致文件夹尾部带空格的处理不正确
        if (!tmp_str.empty()) {
            encodeStr.append(CodecUtil::UrlEncode(tmp_str));
            encodeStr.append(kPathDelimiter);
        }
        pos = next_pos + 1;
    }

    // 如果原路径是文件，则删除最后一个分隔符
    if (dstPath[total_len - 1] != kPathDelimiterChar) {
        encodeStr.erase(encodeStr.length() - 1, 1);
    }
    return encodeStr;
}

uint64_t FileUtil::GetExpiredTime() {
    return time(NULL) + CosSysConfig::getExpiredTime();
}

string FileUtil::FormatPath(const string &path) {
    if(path.empty()) {
        return path;
    }
    string formatStr = kPathDelimiter;
    size_t len = path.length();
    size_t last_delimiter_pos = -1;
    for(size_t i = 0; i < len; ++i) {
        if (path[i] == kPathDelimiterChar && last_delimiter_pos + 1 == i) {
            last_delimiter_pos = i;
            continue;
        } else if (path[i] == kPathDelimiterChar && last_delimiter_pos + 1 < i) {
            last_delimiter_pos = i;
            formatStr.append(1, path[i]);
        } else {
            formatStr.append(1, path[i]);
        }
    }
    return formatStr;
}
}
