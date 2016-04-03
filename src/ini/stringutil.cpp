//#ifndef _STRINGUTIL_CPP
//#define _STRINGUTIL_CPP

#include <string.h>
#include <ctype.h>
#include "stringutil.h"


namespace stringutil
{
	void trimleft(std::string &str,char c/*=' '*/)
	{
		//trim head
	
		int len = str.length();

		int i = 0;
		while(str[i] == c && str[i] != '\0'){
			i++;
		}
		if(i != 0){
			str = std::string(str,i,len-i);
		}
	};
	
	void trimright(std::string &str,char c/*=' '*/)
	{
		//trim tail
		int i = 0;
		int len = str.length();
			
		
		for(i = len - 1; i >= 0; --i ){
			if(str[i] != c){
				break;
			}
		}

		str = std::string(str,0,i+1);
	};

	void trim(std::string &str)
	{
		//trim head
	
		int len = str.length();

		int i = 0;
		while(isspace(str[i]) && str[i] != '\0'){
			i++;
		}
		if(i != 0){
			str = std::string(str,i,len-i);
		}

		//trim tail
		len = str.length();

		for(i = len - 1; i >= 0; --i ){
			if(!isspace(str[i])){
				break;
			}
		}
		str = std::string(str,0,i+1);
	};
}//end of namespace stringutil
//#endif



