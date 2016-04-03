//#ifndef _INIFILE_CPP
//#define _INIFILE_CPP

#include "stringutil.h"
#include "inifile.h"
#include <stdlib.h>
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace inifile{
using namespace stringutil;

int INI_BUF_SIZE=2048;
IniFile::IniFile()
{
	flags_.push_back("#");
	flags_.push_back(";");
}
bool IniFile::parse(const std::string &content,std::string &key,std::string &value,char c/*= '='*/)
{
	int i = 0;
	int len = content.length();

	while(i < len && content[i] != c){
		++i;
	}
	if(i >= 0 && i < len){
		key = std::string(content.c_str(),i);
		trim(key);   //bzh add 2015.7.1
		value = std::string(content.c_str()+i+1,len-i-1);
		trim(value);  //bzh add 2015.7.1
		return true;
	}

	return false;
}

int IniFile::getline(std::string &str,FILE *fp)
{
	int plen = 0;
	int buf_size = INI_BUF_SIZE*sizeof(char);

	char *buf =(char *) malloc(buf_size);
	char *pbuf = NULL;
	char * p = buf;
	
	if(buf == NULL){
		fprintf(stderr,"no enough memory!exit!\n");
		//exit(-1);
		return RET_ERR;
	}
	
	memset(buf,0,buf_size);
	int total_size = buf_size;
	while(fgets(p,buf_size,fp) != NULL){
		plen = strlen(p);

		if( plen > 0 && p[plen-1] != '\n' && !feof(fp)){
	
			total_size = strlen(buf)+buf_size;
			pbuf = (char *)realloc(buf,total_size);
			
			if(pbuf == NULL){
				free(buf);
				fprintf(stderr,"no enough memory!exit!\n");
				//exit(-1);
				return RET_ERR;
			}

			buf = pbuf;
			
			p = buf + strlen(buf);
			
			continue;
		}else{
			break;
		}
	}

	str = buf;
	
	free(buf);
	buf = NULL;
	return str.length();

}

int IniFile::open(const std::string &filename)
{	
	release();
	fname_ = filename;
	IniSection *section = NULL;
	FILE *fp = fopen(filename.c_str(),"r");
	
	if(fp == NULL ){
		return -1;
	}

	std::string line;
	std::string comment;
	
	//增加默认段
	section = new IniSection();
	sections_[""] = section;
	
	while(getline(line,fp) > 0){
		
		trimright(line,'\n');
		trimright(line,'\r');
		trim(line);
		
		if(line.length() <= 0){
			continue;
		}

		if(line[0] == '['){
			section = NULL;
			int index = line.find_first_of(']');

			if(index == -1){
				fclose(fp);
				fprintf(stderr,"没有找到匹配的]\n");
				return -1;
			}
			int len = index-1;
			if(len <= 0){
				fprintf(stderr,"段为空\n");
				continue;
			}
			std::string s(line,1,len);

			if(getSection(s.c_str()) != NULL){
				fclose(fp);
				fprintf(stderr,"此段已存在:%s\n",s.c_str());
				return -1;
			}
			
			section = new IniSection();
			sections_[s] = section;

			section->name = s;
			section->comment = comment;
			comment = "";
		}else if(isComment(line)){
			if(comment != ""){
				comment += delim + line ;
			}else{
				comment = line;
			}
		}else{
			std::string key,value;
			if(parse(line,key,value)){
				IniItem item;
				item.key = key;
				item.value = value;
				item.comment = comment;

				section->items.push_back(item);
			}else{
				fprintf(stderr,"解析参数失败[%s]\n",line.c_str());
			}
			comment = "";
		}
	}
	
	fclose(fp);
	
	return 0;
}

int IniFile::save()
{
	return saveas(fname_);
}

int IniFile::saveas(const std::string &filename)
{
	std::string data = "";
	for(iterator sect = sections_.begin(); sect != sections_.end(); ++sect){
		if(sect->second->comment != ""){
			data += sect->second->comment;	
			data += delim;
		}
		if(sect->first != ""){
			data += std::string("[")+sect->first + std::string("]");	
			data += delim;
		}

		for(IniSection::iterator item = sect->second->items.begin(); item != sect->second->items.end(); ++item){
			if(item->comment != ""){
				data += item->comment;	
				data += delim;
			}
			data += item->key+"="+item->value;
			data += delim;
		}
	}

	FILE *fp = fopen(filename.c_str(),"w");

	fwrite(data.c_str(),1,data.length(),fp);

	fclose(fp);
	
	return 0;
}
IniSection *IniFile::getSection(const std::string &section /*=""*/)
{
	iterator it = sections_.find(section);
	if(it != sections_.end()){
		return it->second;
	}

	return NULL;
}

std::string IniFile::getStringValue(const std::string &section,const std::string &key,int &ret)
{
	std::string value,comment;
	
	ret = getValue(section,key,value,comment);

	return value;
}

int IniFile::getIntValue(const std::string &section,const std::string &key,int &ret)
{
	std::string value,comment;
	
	ret = getValue(section,key,value,comment);
	
	return atoi(value.c_str());
}

int64_t IniFile::getInt64Value(const std::string &section,const std::string &key,int &ret)
{
	std::string value,comment;

	ret = getValue(section,key,value,comment);

	return strtoll(value.c_str(), NULL, 10);
}

double IniFile::getDoubleValue(const std::string &section,const std::string &key,int &ret)
{
	std::string value,comment;
	
	ret = getValue(section,key,value,comment);
	
	return atof(value.c_str());
}

int IniFile::getValue(const std::string &section,const std::string &key,std::string &value)
{
	std::string comment;
	return getValue(section,key,value,comment);
}
int IniFile::getValue(const std::string &section,const std::string &key, std::string &value, std::string &comment)
{
	IniSection * sect = getSection(section);

	if(sect != NULL){
		for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it){
			if(it->key == key){
				value = it->value;
				comment = it->comment;
				return RET_OK;
			}
		}
	}

	return RET_ERR;
}
int IniFile::getValues(const std::string &section,const std::string &key,std::vector<std::string> &values)
{
	std::vector<std::string> comments;
	return getValues(section,key,values,comments);
}
int IniFile::getValues(const std::string &section,const std::string &key,
					   std::vector<std::string> &values,std::vector<std::string> &comments)
{
	std::string value,comment;

	values.clear();
	comments.clear();

	IniSection * sect = getSection(section);

	if(sect != NULL){
		for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it){
			if(it->key == key){
				value = it->value;
				comment = it->comment;
				
				values.push_back(value);
				comments.push_back(comment);
			}
		}
	}

	return (values.size() ? RET_OK : RET_ERR);
}

bool IniFile::hasSection(const std::string &section) 
{
	return (getSection(section) != NULL);
}

bool IniFile::hasKey(const std::string &section,const std::string &key)
{
	IniSection * sect = getSection(section);

	if(sect != NULL){
		for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it){
			if(it->key == key){
				return true;
			}
		}
	}

	return false;
}
int IniFile::getSectionComment(const std::string &section,std::string & comment)
{
	comment = "";
	IniSection * sect = getSection(section);
	
	if(sect != NULL){
		comment = sect->comment;
		return RET_OK;
	}

	return RET_ERR;
}
int IniFile::setSectionComment(const std::string &section,const std::string & comment)
{
	IniSection * sect = getSection(section);
	
	if(sect != NULL){
		sect->comment = comment;
		return RET_OK;
	}

	return RET_ERR;
}

int IniFile::setIntValue(const std::string &section,const std::string &key, int nValue,const std::string &comment)
{
	char pszValue[48] = {0};
	snprintf(pszValue, 48, "%d", nValue);
	return setValue(section, key, pszValue, comment);
}

int IniFile::setInt64Value(const std::string &section,const std::string &key,int64_t nValue,const std::string &comment)
{
	char pszValue[48] = {0};
	snprintf(pszValue, 48, "%" PRIu64 "", nValue);
	return setValue(section, key, pszValue, comment);
}

int IniFile::setValue(const std::string &section,const std::string &key,
					  const std::string &value,const std::string &comment /*=""*/)
{
	IniSection * sect = getSection(section);
	
	std::string comt = comment;
	if (comt != ""){
		comt = flags_[0] +comt;
	} 
	if(sect == NULL){
		sect = new IniSection();
		if(sect == NULL){
			fprintf(stderr,"no enough memory!\n");
			//exit(-1);
			return RET_ERR;
		}
		sect->name = section;
		sections_[section] = sect;
	}
	
	for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it){
		if(it->key == key){
			it->value = value;
			it->comment = comt;
			return RET_OK;
		}
	}

	//not found key
	IniItem item;
	item.key = key;
	item.value = value;
	item.comment = comt;

	sect->items.push_back(item);

	return RET_OK;

}
void IniFile::getCommentFlags(std::vector<std::string> &flags)
{
	flags = flags_;
}
void IniFile::setCommentFlags(const std::vector<std::string> &flags)
{
	flags_ = flags;
}
void IniFile::deleteSection(const std::string &section)
{
	IniSection *sect = getSection(section);

	if(sect != NULL){
	
		sections_.erase(section);	
		delete sect;
	}
}
void IniFile::deleteKey(const std::string &section,const std::string &key)
{
	IniSection * sect = getSection(section);
	
	if(sect != NULL){
		for(IniSection::iterator it = sect->begin(); it != sect->end(); ++it){
			if(it->key == key){
				sect->items.erase(it);
				break;
			}
		}
	}

}

void IniFile::release()
{
	fname_ = "";

	for(iterator i = sections_.begin(); i != sections_.end(); ++i){
		delete i->second;
	}

	sections_.clear();
}

bool IniFile::isComment(const std::string &str)
{
	bool ret =false;
	for(int i = 0; i < flags_.size(); ++i){
		int k = 0;
		if(str.length() < flags_[i].length()){
			continue;
		}
		for(k = 0;k < flags_[i].length(); ++k){
			if(str[k] != flags_[i][k]){
				break;
			}
		}

		if(k == flags_[i].length()){
			ret = true;
			break;
		}
	}

	return ret;
}
//for debug
void IniFile::print()
{
	printf("filename:[%s]\n",fname_.c_str());

	printf("flags_:[");
	for(int i = 0; i < flags_.size(); ++i){
		printf(" %s ",flags_[i].c_str());
	}
	printf("]\n");

	for(iterator it = sections_.begin(); it != sections_.end(); ++it){
		printf("section:[%s]\n",it->first.c_str());
		printf("comment:[%s]\n",it->second->comment.c_str());
		for(IniSection::iterator i = it->second->items.begin(); i != it->second->items.end(); ++i){
			printf("    comment:%s\n",i->comment.c_str());
			printf("    parm   :%s=%s\n",i->key.c_str(),i->value.c_str());
		}
	}
}
}
//#endif
