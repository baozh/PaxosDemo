#include <assert.h>
#include "MsgHandler.h"
#include "CommUtil.h"

MsgHandler::MsgHandler()
	: func_ ()
	, threadType_ (CommServUtil::kDefaultThreadType)
{

}

MsgHandler::MsgHandler(const MsgFunc& func, int threadType)
	: func_ (func)
	, threadType_ (threadType)
{
}
