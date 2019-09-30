
#include <iostream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <cmath>

#include "fileentry.h"
#include "fileentryshared.h"

const int MAX_CHILD_COUNT=100;
const int MIN_CHILD_PIXEL_AREA=50;

FileEntryShared::FileEntryShared(FileEntry::EntryType type):
        id(0), size(0), name(""), entryType(type), isHovered(false), drawArea{} {

}


FileEntryShared::FileEntryShared(const FileEntry& entry, int nestLevel, int64_t minSize, int flags)
{
    reconstruct_from(entry, nestLevel, minSize, flags);
}


FileEntryShared::~FileEntryShared() {

}

void FileEntryShared::init_from(const FileEntry& entry)
{
    id=entry.id;
    size=entry.size;
    name=entry.name;
    isHovered=false;
    isParentHovered=false;
    entryType=entry.get_type();
    drawArea.x=0;
    drawArea.y=0;
    drawArea.w=0;
    drawArea.h=0;
}

void FileEntryShared::reconstruct_from(const FileEntry& entry, int nestLevel, int64_t minSize, uint16_t flags)
{
    init_from(entry);
    parent= nullptr;

    if(nestLevel>0)
    {
        auto child=entry.firstChild;
        size_t childCount = 0;
        size_t existingChildCount = children.size();
        while (child!= nullptr)
        {
            if(childCount>=MAX_CHILD_COUNT)
                break;

            auto type=child->get_type();

            if(child->get_size()<minSize)
                break;
            if((type!=FileEntry::AVAILABLE_SPACE || (flags&INCLUDE_AVAILABLE_SPACE)!=0) &&
               (type!=FileEntry::UNKNOWN_SPACE || (flags&INCLUDE_UNKNOWN_SPACE)!=0))
            {
                if(childCount<existingChildCount)
                {
                    update_copy(children[childCount], *child, nestLevel - 1, minSize, flags);
                    children[childCount]->parent = this;
                } else
                {
                    auto newChild = create_copy(*child, nestLevel - 1, minSize, flags);
                    newChild->parent = this;
                    children.push_back(newChild);
                }

                ++childCount;
            }

            child=child->nextEntry;
        }

        while(existingChildCount>childCount)
        {
            children.pop_back();
            --existingChildCount;
        }

    } else
    {
        children.clear();
    }
}

std::string FileEntryShared::get_title()
{
    std::string sizeStr=format_size();
    return string_format("%s - %s",name.c_str(),sizeStr.c_str());
}

std::string FileEntryShared::get_tooltip() const
{
    std::string sizeStr=format_size();
    std::string str;

    if(is_dir())
        str=string_format("<center>%s<br />%s<br />(Click to zoom)</center>",name.c_str(),sizeStr.c_str());
    else if(is_file())
        str=string_format("<center>%s<br />%s</center>",name.c_str(),sizeStr.c_str());

    return str;
}

FileEntryShared* FileEntryShared::update_hovered_element(float mouseX, float mouseY)
{
    FileEntryShared* hoveredEntry = nullptr;
    if(drawArea.x<=mouseX && drawArea.y<=mouseY &&
            drawArea.w>=mouseX-drawArea.x && drawArea.h>=mouseY-drawArea.y)
    {
        hoveredEntry = this;
        isHovered=true;
        isParentHovered=true;

        FileEntryShared* hoveredChild=nullptr;

        for(const auto& child :children)
        {
            if(hoveredChild)
                child->set_hovered(false);
            else
                hoveredChild=child->update_hovered_element(mouseX, mouseY);
            child->set_parent_hovered(true);
        }
        if(hoveredChild)
        {
            hoveredEntry=hoveredChild;
            isHovered= false;
            isParentHovered=true;
        }
    } else
    {
        isHovered=false;
        for(const auto& child :children)
        {
            child->set_parent_hovered(false);
            child->set_hovered(false);
        }
    }
    
    return hoveredEntry;
}

void FileEntryShared::set_hovered(bool hovered)
{
    if(isHovered==hovered)
        return;
    
    isHovered=hovered;
    
    for(const auto& child :children)
    {
        child->set_hovered(hovered);
    }
}

void FileEntryShared::set_parent_hovered(bool hovered)
{
    isParentHovered=hovered;
    
    for(const auto& child :children)
    {
        child->set_parent_hovered(hovered);
    }
}

std::string FileEntryShared::format_size() const
{
    return Utils::format_size(size);
}

void FileEntryShared::unhover()
{
    isHovered=false;
    isParentHovered=false;
    if(parent)
        parent->unhover();
}

void FileEntryShared::allocate_children(Utils::RectI rect, int titleHeight)
{
    if(children.empty() || rect.h<titleHeight || rect.w<1)
        return;
    
    drawArea=rect;
    
    rect.y+=titleHeight+2;
    rect.h-=titleHeight+2+2;
    rect.x+=2;
    rect.w-=4;
    
    
    allocate_children2(0, children.size()-1, children, rect);
    for(const auto& child:children)
    {
        child->allocate_children(child->drawArea, titleHeight);
    }
}

const char* FileEntryShared::get_name() {
    switch(entryType)
    {
        case FileEntry::AVAILABLE_SPACE:
            return "Free";
        case FileEntry::UNKNOWN_SPACE:
            return "Unknown";
        default:
            return name.c_str();
    }
}

void FileEntryShared::allocate_children2(size_t start, size_t end, std::vector<FileEntrySharedPtr> &bin, Utils::RectI &rect)
{
    if(rect.h<1 || rect.w<1)
        return;
    if(end-start<0)
        return;
    else if((end-start)==0)
    {
        set_child_rect(bin[start],rect);
        return;
    }

    int64_t totalSize=0;
    for(size_t i=start;i<=end;++i)
    {
        totalSize+=bin[i]->size;
    }

    int availableArea = rect.h * rect.w;
    auto minPart = MIN_CHILD_PIXEL_AREA / availableArea;
    auto minSize = int64_t(totalSize*minPart);
    totalSize=0;
    size_t newEnd=start;
    for(size_t i=start;i<=end;++i)
    {
        if(bin[i]->size<minSize && i>start+1)
            break;
        totalSize+=bin[i]->size;
        ++newEnd;
    }
    for(size_t i=newEnd;i<=end;++i)
    {
        bin[i]->drawArea.w=0.f;
        bin[i]->drawArea.h=0.f;
    }
    if(newEnd==start)
        return;
    end = newEnd-1;
    
    size_t bin1last=start;
    int64_t sum1=bin[start]->size;
    int64_t sum2=0;

    for(size_t i=start+1;i<=end;++i)
        sum2+=bin[i]->size;
    
    bool bin1Filled=false;
    
    for(size_t i=start+1;i<=end;++i)
    {
        auto curSize=bin[i]->size;
        if(!bin1Filled && std::abs(sum2-sum1- curSize*2)<std::abs(sum2-sum1))
        {
            ++bin1last;
            sum1+=curSize;
            sum2-=curSize;
        } else{
            bin1Filled=true;
        }
    }
    
    if(sum1==0 && sum2==0)
    {
        sum1=1;
        sum2=1;
    }
    
    //float a1=float(sum1)/float(sum1+sum2);
    
    
    bool divX=rect.w>=rect.h;
    
    int rX=rect.x;
    int rY=rect.y;
    int64_t rW=rect.w;
    int64_t rH=rect.h;
    int midW1= static_cast<int>((rW*sum1)/(sum1+sum2));
    int midH1= static_cast<int>((rH*sum1)/(sum1+sum2));
    
    if(divX)
    {
        Utils::RectI rect2{};
        rect2.x=rX;
        rect2.y=rY;
        rect2.w=midW1;
        rect2.h=rH;
        
        if((bin1last-start)>0)
            allocate_children2(start, bin1last, bin, rect2);
        else
            set_child_rect(bin[start], rect2);

        rect2.x=rX+midW1;
        rect2.w=static_cast<int>(rW-midW1);
        if((end-bin1last)>1)
            allocate_children2(bin1last+1, end, bin, rect2);
        else
            set_child_rect(bin[end], rect2);
    }
    else
    {
        Utils::RectI rect2{};
        rect2.x=rX;
        rect2.y=rY;
        rect2.w=rW;
        rect2.h=midH1;
        
        if((bin1last-start)>0)
            allocate_children2(start, bin1last, bin, rect2);
        else
            set_child_rect(bin[start], rect2);

        rect2.y=rY+midH1;
        rect2.h=static_cast<int>(rH-midH1);
        if((end-bin1last)>1)
            allocate_children2(bin1last+1, end, bin, rect2);
        else
            set_child_rect(bin[end], rect2);
    }
}

void FileEntryShared::set_child_rect(const FileEntrySharedPtr& child, Utils::RectI &rect)
{
    child->drawArea=rect;
    
    if(child->drawArea.w<1 || child->drawArea.h<1)
    {
        child->drawArea.w=0;
        child->drawArea.h=0;
    }
}

FileEntrySharedPtr FileEntryShared::create_copy(const FileEntry& entry, int nestLevel, int64_t minSize, int flags)
{
    return FileEntrySharedPtr(new FileEntryShared(entry, nestLevel, minSize, flags));
}

void FileEntryShared::update_copy(FileEntrySharedPtr& copy, const FileEntry& entry, int nestLevel, int64_t minSize, int flags)
{
    copy->reconstruct_from(entry, nestLevel, minSize, flags);
}

const char* FileEntryShared::get_path(bool countRoot) {
    if(parent)
    {
        auto ppath=parent->get_path(countRoot);
        auto pathLen=strlen(ppath);
        auto path=new char[pathLen+name.length()+5];

        if(pathLen == 0 || ppath[pathLen-1]=='/' || ppath[pathLen-1]=='\\')
            sprintf(path,"%s%s",ppath, name.c_str());
        else
            sprintf(path,"%s/%s",ppath, name.c_str());

        delete[](ppath);
        return path;

    }
    else
    {
        auto path=new char[name.length()+2];
        if(countRoot)
            strcpy(path,name.c_str());
        else
            path[0] = '\0';

        return path;
    }

//    return path;
}


const std::vector<FileEntrySharedPtr>& FileEntryShared::get_children() {
    return children;
}
//
//FileEntryPtr FileEntry::create_shared_copy(int nestLevel)
//{
//    auto len=strlen(name)+1;
//    char* name2=new char[len];
//    memcpy(name2, name, (len)*sizeof(char)); // int is a POD
//    auto newRoot=new FileEntry(0,name2,isDir);
//
//    for(auto child : children)
//    {
//        const char* name=child->get_name();
//        auto len=strlen(name)+1;
//        char* name2=new char[len];
//        memcpy(name2, name, (len)*sizeof(char)); // int is a POD
//        auto newChild=new FileEntry(0,name2,child->is_dir());
//        newChild->set_size(child->get_size());
//        newRoot->add_child(newChild);
//    }
//
//    return FileEntryPtr(newRoot);
//}
