#ifndef _INIFILE_H
#define _INIFILE_H

#include <map>
#include <vector>
#include <string>
#include <string.h>
#include <stdint.h>




//using namespace std;
namespace inifile
{
const int RET_OK  = 0;
const int RET_ERR = -1;
const std::string delim = "\n";
struct IniItem
{
	std::string key;
	std::string value;
	std::string comment;
};
struct IniSection
{
	typedef std::vector<IniItem>::iterator iterator;
	iterator begin() {return items.begin();}
	iterator end() {return items.end();}

	std::string name;
	std::string comment;
	std::vector<IniItem> items;
};

class IniFile
{
public:	
	IniFile();
	~IniFile(){release();}

public:
	typedef std::map<std::string,IniSection *>::iterator iterator;

	iterator begin() {return sections_.begin();}
	iterator end() {return sections_.end();}
public:
	///* 打开并解析一个名为fname的INI文件 */
	int open(const std::string &fname);
	///*将内容保存到当前文件*/
	int save();
	///*将内容另存到一个名为fname的文件*/
	int saveas(const std::string &fname);
	
	///*获取section段第一个键为key的值,并返回其string型的值*/
	std::string getStringValue(const std::string &section,const std::string &key,int &ret);
	///*获取section段第一个键为key的值,并返回其int型的值*/
	int getIntValue(const std::string &section,const std::string &key,int &ret);

	int64_t getInt64Value(const std::string &section,const std::string &key,int &ret);
	///*获取section段第一个键为key的值,并返回其double型的值*/
	double getDoubleValue(const std::string &section,const std::string &key,int &ret);
	
	///*获取section段第一个键为key的值,并将值赋到value中*/
	int getValue(const std::string &section,const std::string &key,std::string &value);
	///*获取section段第一个键为key的值,并将值赋到value中,将注释赋到comment中*/
	int getValue(const std::string &section,const std::string &key,std::string &value,std::string &comment);
	
	///*获取section段所有键为key的值,并将值赋到values的vector中*/
	int getValues(const std::string &section,const std::string &key,std::vector<std::string> &values);
	///*获取section段所有键为key的值,并将值赋到values的vector中,,将注释赋到comments的vector中*/
	int getValues(const std::string &section,const std::string &key,std::vector<std::string> &value,std::vector<std::string> &comments);
	
	bool hasSection(const std::string &section) ;
	bool hasKey(const std::string &section,const std::string &key) ;
	
	///* 获取section段的注释 */
	int getSectionComment(const std::string &section,std::string & comment);
	///* 设置section段的注释 */
	int setSectionComment(const std::string &section,const std::string & comment);
	///*获取注释标记符列表*/
	void getCommentFlags(std::vector<std::string> &flags);
	///*设置注释标记符列表*/
	void setCommentFlags(const std::vector<std::string> &flags);
	
	///*同时设置值和注释*/
	int setValue(const std::string &section,const std::string &key,const std::string &value,const std::string &comment="");
	int setIntValue(const std::string &section,const std::string &key,int nValue,const std::string &comment="");
	int setInt64Value(const std::string &section,const std::string &key,int64_t nValue,const std::string &comment="");
	///*删除段*/
	void deleteSection(const std::string &section);
	///*删除特定段的特定参数*/
	void deleteKey(const std::string &section,const std::string &key);
private:
	IniSection *getSection(const std::string &section="");
	void release();
	int getline(std::string &str,FILE *fp);
	bool isComment(const std::string &str);
	bool parse(const std::string &content,std::string &key,std::string &value,char c= '=');
	////for dubug
	void print();
private:
	std::map<std::string,IniSection *> sections_;
	std::string fname_;
	std::vector<std::string> flags_;
};

}

#endif
