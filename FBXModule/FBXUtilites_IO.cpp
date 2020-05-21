/**
* @file FBXUtilites_IO.cpp
* @date 2018/06/15
* @author Lim Taewoo (limztudio@gmail.com)
*/


#include "stdafx.h"

#include <algorithm>

#include "FBXUtilites.h"


using namespace fbxsdk;


CustomStream::CustomStream(FbxManager* kSDKManager, const char* fileName, const char* mode)
    :
    m_fileName(fileName),
    m_fileMode(mode),

    m_file(nullptr)
{
    if(m_fileName.empty())
        return;

    if(m_fileMode.empty())
        return;

    std::transform(m_fileMode.begin(), m_fileMode.end(), m_fileMode.begin(), tolower);

    if(*m_fileMode.cbegin() == 'r'){
        static const char format[] = "FBX (*.fbx)";
        m_readerID = kSDKManager->GetIOPluginRegistry()->FindReaderIDByDescription(format);
        m_writerID = -1;
    }
    else if(*m_fileMode.cbegin() == 'w'){
        static const char format[] = "FBX ascii (*.fbx)";
        m_writerID = kSDKManager->GetIOPluginRegistry()->FindWriterIDByDescription(format);
        m_readerID = -1;
    }
}
CustomStream::~CustomStream(){
    Close();
}

bool CustomStream::Open(void* /*streamData*/){
    if(!m_file)
        fopen_s(&m_file, m_fileName.c_str(), m_fileMode.c_str());

    if(m_file != nullptr)
        _fseeki64(m_file, 0, SEEK_SET);

    return (m_file != nullptr);
}
bool CustomStream::Close(){
    if(!m_file)
        return false;

    fclose(m_file);
    m_file = nullptr;

    return true;
}

int CustomStream::Write(const void* data, int size){
    if(!m_file)
        return 0;

    return ((int)fwrite(data, 1, size, m_file));
}
int CustomStream::Read(void* data, int size)const{
    if(!m_file)
        return 0;

    return ((int)fread_s(data, size , 1, size, m_file));
}

void CustomStream::Seek(const FbxInt64& offset, const FbxFile::ESeekPos& seekPos){
    if(!m_file)
        return;

    switch(seekPos){
    case FbxFile::eBegin:
        _fseeki64(m_file, offset, SEEK_SET);
        break;
    case FbxFile::eCurrent:
        _fseeki64(m_file, offset, SEEK_CUR);
        break;
    case FbxFile::eEnd:
        _fseeki64(m_file, offset, SEEK_END);
        break;
    }
}

long CustomStream::GetPosition()const{
    if(!m_file)
        return 0;

    return (long)_ftelli64(m_file);
}
void CustomStream::SetPosition(long position){
    if(!m_file)
        return;

    _fseeki64(m_file, position, SEEK_SET);
}

int CustomStream::GetError()const{
    if(!m_file)
        return 0;

    return ferror(m_file);
}
void CustomStream::ClearError(){
    if(!m_file)
        return;

    clearerr(m_file);
}
