#include <locale>
#include "./locale.h"

namespace rz {

void setUTF8() {
	// 設定全局 locale
	std::locale::global( std::locale( "en_US.UTF-8" ) );

}

}
