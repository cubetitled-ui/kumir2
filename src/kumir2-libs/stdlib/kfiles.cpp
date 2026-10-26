#include "kumirstdlib.hpp"

#if defined(WIN32) || defined(_WIN32)
# define NOMINMAX
# include <Windows.h>
#else
# include <sys/stat.h>
# include <errno.h>
# include <unistd.h>
#endif

#include <string.h>
#include <vector>
#include <algorithm>

namespace Kumir {

static FILE *assignedIN = stdin;
static FILE *assignedOUT = stdout;

static std::deque<FileType> openedFiles;

static AbstractInputBuffer *consoleInputBuffer = 0;
static AbstractOutputBuffer *consoleOutputBuffer = 0;

static Encoding fileEncoding;
static String inputDelimiters;

#if defined(WIN32) || defined(_WIN32)
static Encoding LOCALE_ENCODING = CP866;
#else
static Encoding LOCALE_ENCODING = UTF8;
#endif


AbstractInputBuffer::~AbstractInputBuffer()
{
}

AbstractOutputBuffer::~AbstractOutputBuffer()
{
}


AbstractInputBuffer *Files::getConsoleInputBuffer()
{
	return consoleInputBuffer;
}

void Files::setConsoleInputBuffer(AbstractInputBuffer *b)
{
	consoleInputBuffer = b;
}

AbstractOutputBuffer *Files::getConsoleOutputBuffer()
{
	return consoleOutputBuffer;
}

void Files::setConsoleOutputBuffer(AbstractOutputBuffer *b)
{
	consoleOutputBuffer = b;
}

bool Files::isOpenedFiles()
{
	bool remainingOpenedFiles = false;
	for (
		std::deque<FileType>::iterator it = openedFiles.begin();
		it != openedFiles.end();
		++it
	) {
		FileType &f = *it;
		if (!f.autoClose) {
			remainingOpenedFiles = true;
			break;
		}
	}
	return remainingOpenedFiles;
}

void Files::init()
{
	fileEncoding = DefaultEncoding;
	openedFiles.clear();

	assignedIN = stdin;
	assignedOUT = stdout;
	inputDelimiters = Kumir::Core::fromAscii(" \n\r\t");
}

void Files::finalize()
{
	if (isOpenedFiles() && Core::getError().length() == 0) {
		Core::abort(Core::fromUtf8("Остались незакрытые файлы"));
	}

	for (size_t i = 0; i < openedFiles.size(); i++) {
		FileType &f = openedFiles[i];
		if (f.handle) {
			fclose(f.handle);
		}
	}

	openedFiles.clear();

	if (assignedIN != stdin) {
		fclose(assignedIN);
	}

	if (assignedOUT != stdout) {
		fclose(assignedOUT);
	}

	assignedIN = stdin;
	assignedOUT = stdout;
}

void Files::setFileEncoding(const String &enc)
{
	String encoding = Core::toLowerCaseW(enc);
	StringUtils::trim<String, Char>(encoding);
	if (encoding.length() == 0) {
		fileEncoding = DefaultEncoding;
		return;
	}

	size_t minus = encoding.find_first_of(Char('-'));
	if (minus != String::npos) {
		encoding.erase(minus, 1);
	}

	static const String ansi1 = Core::fromAscii("cp1251");
	static const String ansi2 = Core::fromAscii("windows1251");
	static const String ansi3 = Core::fromAscii("windows");
	static const String ansi4 = Core::fromAscii("ansi");
	static const String ansi5 = Core::fromAscii("1251");
	static const String oem1 = Core::fromAscii("cp866");
	static const String oem2 = Core::fromAscii("ibm866");
	static const String oem3 = Core::fromAscii("ibm");
	static const String oem4 = Core::fromAscii("oem");
	static const String oem5 = Core::fromAscii("oem866");
	static const String oem6 = Core::fromAscii("dos");
	static const String koi1 = Core::fromAscii("koi8");
	static const String koi2 = Core::fromAscii("koi8r");
	static const String koi3 = Core::fromUtf8("кои8");
	static const String koi4 = Core::fromUtf8("кои8р");
	static const String utf1 = Core::fromAscii("utf");
	static const String utf2 = Core::fromAscii("utf8");
	static const String utf3 = Core::fromAscii("linux");
	static const String intel1 = Core::fromAscii("unicode");
	static const String intel2 = Core::fromAscii("utf16");
	static const String intel3 = Core::fromAscii("utf16le");
	static const String intel4 = Core::fromUtf8("юникод");
	static const String motorola = Core::fromAscii("utf16be");

	if (
		encoding == ansi1 || encoding == ansi2 || encoding == ansi3 ||
		encoding == ansi4 || encoding == ansi5
	) {
		fileEncoding = CP1251;
	} else if (
		encoding == oem1 || encoding == oem2 || encoding == oem3 ||
		encoding == oem4 || encoding == oem5 || encoding == oem6
	) {
		fileEncoding = CP866;
	} else if (
		encoding == koi1 || encoding == koi2 || encoding == koi3 ||
		encoding == koi4
	) {
		fileEncoding = KOI8R;
	} else if (
		encoding == utf1 || encoding == utf2 || encoding == utf3
	) {
		fileEncoding = UTF8;
	} else if (
		encoding == intel1 || encoding == intel2 || encoding == intel3 ||
		encoding == intel4
	) {
		fileEncoding = UTF16INTEL;
	} else if (
		encoding == motorola
	) {
		fileEncoding = UTF16MOTOROLA;
	} else {
		Core::abort(Core::fromUtf8("Неизвестная кодировка"));
	}
}

String Files::getAbsolutePath(const String &fileName)
{
#if defined(WIN32) || defined(_WIN32)
	wchar_t cwd[1024];
	GetCurrentDirectoryW(1024, cwd);
	String workDir;
	workDir = String(cwd);
	workDir.push_back(Char('\\'));
	String absPath;
	if (
		fileName.length() == 0 ||
		(fileName.length() > 2 && fileName.at(1) == Char(':') && fileName.at(2) == Char('\\'))
	) {
		absPath = fileName;
	} else {
		absPath = workDir + fileName;
	}
	return getNormalizedPath(absPath, Char('\\'));
#else
	char cwd[1024];
	getcwd(cwd, 1024 * sizeof(char));
	String workDir;
	std::string sworkDir = std::string(cwd);
	workDir = Core::fromUtf8(sworkDir);
	workDir.push_back(Char('/'));
	String absPath;
	if (fileName.length() == 0 || fileName.at(0) == Char('/')) {
		absPath = fileName;
	} else {
		absPath = workDir + fileName;
	}
	return getNormalizedPath(absPath, Char('/'));
#endif
}

String Files::CurrentWorkingDirectory()
{
#if defined(WIN32) || defined(_WIN32)
	wchar_t cwd[1024];
	GetCurrentDirectoryW(1024, cwd);
	String workDir;
	workDir = String(cwd);
	return workDir;
#else
	char cwd[1024];
	getcwd(cwd, 1024 * sizeof(char));
	String workDir;
	std::string sworkDir = std::string(cwd);
	workDir = Core::fromUtf8(sworkDir);
	return workDir;
#endif
}

String Files::getNormalizedPath(const String &path, Char sep)
{
	if (path.length() == 0) {
		return path;
	}
	StringList parts = Core::splitString(path, sep, true);
	StringList normParts;
	int skip = 0;
	String result;
	static const String CUR = Core::fromAscii(".");
	static const String UP  = Core::fromAscii("..");
	for (int i = parts.size() - 1; i >= 0; i--) {
		const String &apart = parts[i];
		if (apart == CUR) {
			// pass
		} else if (apart == UP) {
			skip += 1;
		} else {
			if (skip == 0) {
				normParts.push_front(apart);
			} else {
				skip -= 1;
			}
		}
	}
	result = normParts.join(sep);
	if (path.at(0) == sep) {
		result.insert(0, 1, sep);
	}
	if (path.length() > 1 && path.at(path.length() - 1) == sep) {
		result.push_back(sep);
	}
	return result;
}

bool Files::canOpenForRead(const String &fileName)
{
#if defined(WIN32) || defined(_WIN32)
	return exist(fileName); // TODO implement me
#else
	char *path = reinterpret_cast<char *>(calloc(fileName.length() * 2 + 1, sizeof(char)));
	size_t pl = wcstombs(path, fileName.c_str(), fileName.length() * 2 + 1);
	path[pl] = '\0';
	struct stat st;
	int res = stat(path, &st);
	free(path);

	bool exists = (res == 0);
	bool readAsOwner = false;
	bool readAsGroup = false;
	bool readAsOther = false;
	if (exists) {
		bool w = st.st_mode & S_IRUSR;
		bool g = st.st_mode & S_IRGRP;
		bool o = st.st_mode & S_IROTH;
		readAsOwner = w && getuid() == st.st_uid;
		readAsGroup = g && getgid() == st.st_gid;
		readAsOther = o;
	}
	bool result = readAsOwner || readAsGroup || readAsOther;
	return result;
#endif
}

bool Files::canOpenForWrite(const String &fileName)
{
#if defined(WIN32) || defined(_WIN32)
	return true; // TODO implement me
#else

	char *path = reinterpret_cast<char *>(calloc(fileName.length() * 2 + 1, sizeof(char)));
	size_t pl = wcstombs(path, fileName.c_str(), fileName.length() * 2 + 1);
	path[pl] = '\0';
	struct stat st;
	int res = stat(path, &st);
	bool exists = res == 0;
	bool writeAsOwner = false;
	bool writeAsGroup = false;
	bool writeAsOther = false;
	if (exists) {
		bool w = st.st_mode & S_IRUSR;
		bool g = st.st_mode & S_IRGRP;
		bool o = st.st_mode & S_IROTH;
		writeAsOwner = w && getuid() == st.st_uid;
		writeAsGroup = g && getgid() == st.st_gid;
		writeAsOther = o;
	} else if (errno == ENOENT) {
		while (!exists) {
			int i = 0;
			for (i = strlen(path) - 1; i >= 0; i++) {
				if (path[i] == '/') {
					path[i] = '\0';
					break;
				}
			}
			if (i == 0) {
				break;
			}
			res = stat(path, &st);
			exists = res == 0;
			if (exists) {
				bool w = st.st_mode & S_IRUSR;
				bool g = st.st_mode & S_IRGRP;
				bool o = st.st_mode & S_IROTH;
				writeAsOwner = w && getuid() == st.st_uid;
				writeAsGroup = g && getgid() == st.st_gid;
				writeAsOther = o;
				break;
			}
		}
	}
	free(path);
	bool result = writeAsOwner || writeAsGroup || writeAsOther;
	return result;
#endif
}

bool Files::exist(const String &fileName)
{
#if defined(WIN32) || defined(_WIN32)
	DWORD dwAttrib = GetFileAttributesW(fileName.c_str());
	return dwAttrib != INVALID_FILE_ATTRIBUTES;
#else
	EncodingError encodingError;
	std::string localName = Kumir::Coder::encode(Kumir::UTF8, fileName, encodingError);
	const char *path = localName.c_str();
	struct stat st;
	int res = stat(path, &st);
	bool result = (res == 0);
	return result;
#endif
}

bool Files::isDirectory(const String &fileName)
{
#if defined(WIN32) || defined(_WIN32)
	DWORD dwAttrib = GetFileAttributesW(fileName.c_str());
	return (
		(dwAttrib != INVALID_FILE_ATTRIBUTES) &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
	);
#else
	char *path = reinterpret_cast<char *>(calloc(fileName.length() * 2 + 1, sizeof(char)));
	size_t pl = wcstombs(path, fileName.c_str(), fileName.length() * 2 + 1);
	path[pl] = '\0';
	struct stat st;
	int res = stat(path, &st);
	free(path);
	bool result = (res == 0) && S_ISDIR(st.st_mode);
	return result;
#endif
}

bool Files::mkdir(const String &fileName)
{
#if defined(WIN32) || defined(_WIN32)
	BOOL result = CreateDirectoryW(fileName.c_str(), NULL);
	return result != 0;
#else
	char *path = reinterpret_cast<char *>(calloc(fileName.length() * 2 + 1, sizeof(char)));
	size_t pl = wcstombs(path, fileName.c_str(), fileName.length() * 2 + 1);
	path[pl] = '\0';
	int res = ::mkdir(path, 0666);
	free(path);
	bool result = (res == 0);
	return result;
#endif
}

bool Files::unlink(const String &fileName)
{
#if defined(WIN32) || defined(_WIN32)
	if (DeleteFileW(fileName.c_str()) != 0) {
		return true;
	} else {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			return false;
		} else {
			return false;
		}
	}
#else
	char *path = reinterpret_cast<char *>(calloc(fileName.length() * 2 + 1, sizeof(char)));
	size_t pl = wcstombs(path, fileName.c_str(), fileName.length() * 2 + 1);
	path[pl] = '\0';
	int res = ::unlink(path);
	free(path);
	bool result = (res == 0);
	return result;
#endif
}

bool Files::rmdir(const String &fileName)
{
#if defined(WIN32) || defined(_WIN32)
	if (RemoveDirectoryW(fileName.c_str())) {
		return true;
	} else {
		return false;
	}
#else
	char *path = reinterpret_cast<char *>(calloc(fileName.length() * 2 + 1, sizeof(char)));
	size_t pl = wcstombs(path, fileName.c_str(), fileName.length() * 2 + 1);
	path[pl] = '\0';
	int res = ::rmdir(path);
	free(path);
	bool result = (res == 0);
	return result;
#endif
}


FileType Files::getConsoleBuffer()
{
	if (!consoleInputBuffer) {
		Core::abort(Core::fromUtf8("Консоль не доступна"));
		return FileType();
	} else {
		FileType ft;
		ft.valid = true;
		ft.setType(FileType::Console);
		return ft;
	}
}

FileType Files::open(
	const String &shortName,
	FileType::OpenMode mode,
	bool remember,
	FILE **fh
) {
	String fileName = getAbsolutePath(shortName);
	for (std::deque<FileType>::const_iterator it = openedFiles.begin(); it != openedFiles.end(); ++it) {
		const FileType &f = (*it);
		if (f.getName() == fileName) {
			Core::abort(Core::fromUtf8("Файл уже открыт: ") + fileName);
			return FileType();
		}
	}
	bool isCorrectName = true;
	std::string localName;
#if !defined(WIN32) && !defined(_WIN32)
	EncodingError encodingError;
	localName = Coder::encode(UTF8, fileName, encodingError);
	isCorrectName = NoEncodingError == encodingError;
#endif
	if (!isCorrectName) {
		Kumir::Core::abort(Kumir::Core::fromUtf8("Ошибка открытия файла: имя содержит недопустимый символ"));
		return FileType();
	}
	const char *path = localName.c_str();
	FILE *res = 0;

#if defined(WIN32) || defined(_WIN32)
	const wchar_t *fmode = 0;
	if (mode == FileType::Read) {
		fmode = L"rb";
	} else if (mode == FileType::Write) {
		fmode = L"wb";
	} else if (mode == FileType::Append) {
		fmode = L"ab";
	}
	res = _wfopen(fileName.c_str(), fmode);
#else
	const char *fmode = 0;
	if (mode == FileType::Read) {
		fmode = "rb";
	} else if (mode == FileType::Write) {
		fmode = "wb";
	} else if (mode == FileType::Append) {
		fmode = "ab";
	}
	res = fopen(path, fmode);
#endif

	bool file_not_exists = ENOENT == errno;
	FileType f;
	if (res == 0) {
		if (file_not_exists) {
			Core::abort(Core::fromUtf8("Файл не существует: ") + fileName);
		} else {
			Core::abort(Core::fromUtf8("Невозможно открыть файл: ") + fileName);
		}
	} else {
		if (mode == FileType::Append) {
			mode = FileType::Write;
		}
		f.setName(fileName);
		f.setMode(mode);
		f.handle = res;
		f.autoClose = !remember;
		openedFiles.push_back(f);
		if (fh) {
			*fh = res;
		}
	}
	return f;
}

void Files::close(const FileType &key)
{
	std::deque<FileType>::iterator it = openedFiles.begin();
	for (; it != openedFiles.end(); ++it) {
		FileType f = (*it);
		if (f == key) {
			break;
		}
	}
	if (it == openedFiles.end()) {
		Core::abort(Core::fromUtf8("Неверный ключ"));
		return;
	}
	FileType f = (*it);
	FILE *fh = f.handle;
	f.invalidate();
	if (fh) {
		fclose(fh);
	}
	openedFiles.erase(it);
}

void Files::reset(FileType &key)
{
	std::deque<FileType>::iterator it = openedFiles.begin();
	for (; it != openedFiles.end(); ++it) {
		const FileType &f = (*it);
		if (f == key) {
			break;
		}
	}
	if (it == openedFiles.end()) {
		Core::abort(Core::fromUtf8("Неверный ключ"));
		return;
	}
	FILE *fh = it->handle;
	fseek(fh, 0, 0);
}

bool Files::eof(const FileType &key)
{
	std::deque<FileType>::iterator it = openedFiles.begin();
	for (; it != openedFiles.end(); ++it) {
		const FileType &f = (*it);
		if (f == key) {
			break;
		}
	}
	if (it == openedFiles.end()) {
		Core::abort(Core::fromUtf8("Неверный ключ"));
		return false;
	}
	FILE *fh = it->handle;

	if (feof(fh)) {
		return true;
	}

	int ch = fgetc(fh);
	if (ch < 0) {
		return true;
	}

	ungetc(ch, fh);
	return false;
}

bool Files::hasData(const FileType &key)
{
	std::deque<FileType>::iterator it = openedFiles.begin();
	for (; it != openedFiles.end(); ++it) {
		const FileType &f = (*it);
		if (f == key) {
			break;
		}
	}

	if (it == openedFiles.end()) {
		Core::abort(Core::fromUtf8("Неверный ключ"));
		return false;
	}
	FILE *fh = it->handle;
#if 1
	for (;;) {
		int c = fgetc(fh);
		if (c < 0) {
			return false;
		}
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
			continue;
		}
		ungetc(c, fh);
		return true;
	}

#else
	long backPos = -1;
	if (fh != stdin) {
		backPos = ftell(fh);
	}
	std::vector<char> buffer(1024);
	size_t readAmount = 0;
	bool result = false;
	while (1) {
		if (feof(fh)) {
			break;
		}
		char ch = fgetc(fh);
		if ((unsigned char)(ch) == 0xFF) {
			break;
		}
		if (fh == stdin) {
			if (readAmount >= buffer.size()) {
				buffer.resize(buffer.size() * 2);
			}
			buffer[readAmount] = ch;
			readAmount ++;
		}
		if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
			// found a whitespace
		} else {
			result = true;
			break;
		}
	}
	if (fh == stdin) {
		for (int i = readAmount - 1; i >= 0; i--) {
			ungetc(buffer[i], fh);
		}
	} else {
		fseek(fh, backPos, SEEK_SET);
	}
	return result;
#endif
}

bool Files::overloadedStdIn()
{
	return assignedIN != stdin;
}

bool Files::overloadedStdOut()
{
	return assignedOUT != stdout;
}

FILE *Files::getAssignedIn()
{
	return assignedIN;
}

FILE *Files::getAssignedOut()
{
	return assignedOUT;
}

void Files::assignInStream(String fileName)
{
	StringUtils::trim<String, Char>(fileName);
	if (assignedIN != stdin) {
		fclose(assignedIN);
	}
	if (fileName.length() > 0) {
		open(fileName, FileType::Read, false, &assignedIN);
	} else {
		assignedIN = stdin;
	}
}

void Files::assignOutStream(String fileName)
{
	StringUtils::trim<String, Char>(fileName);
	if (assignedIN != stdout) {
		fclose(assignedOUT);
	}
	if (fileName.length() > 0) {
		open(fileName, FileType::Write, false, &assignedOUT);
	} else {
		assignedOUT = stdout;
	}
}

IO::OutputStream::OutputStream(FILE *f, Encoding enc)
{
	streamType_ = File;
	file = f;
	encoding = enc;
	if (encoding == DefaultEncoding) {
		encoding = UTF8;
	}
	externalBuffer_ = 0;

	if (encoding == UTF8 && ftell(file) == 0) {
		static const char *BOM = "\xEF\xBB\xBF";
		fwrite(BOM, sizeof(char), 3, file);
	}
}

void IO::OutputStream::writeRawString(const String &s)
{
	if (type() == File) {
		EncodingError encodingError;
		std::string bytes = Coder::encode(encoding, s, encodingError);
		if (encodingError) {
			Core::abort(Core::fromUtf8("Ошибка кодирования строки вывода: недопустимый символ"));
		}
		fwrite(bytes.c_str(), sizeof(char), bytes.length(), file);
	} else if (type() == ExternalBuffer) {
		if (!externalBuffer_) {
			Core::abort(Core::fromUtf8("Ошибка вывода: консоль не доступна"));
		} else {
			externalBuffer_->writeRawString(s);
		}
	} else {
		buffer.append(s);
	}
}

IO::InputStream::InputStream(FILE *f, Encoding enc)
{
	streamType_ = File;
	file_ = f;
	externalBuffer_ = 0;
	encoding_ = enc;
	lastChar_ = 0;
	lastCharLength_ = 0;
	lastCharHere_ = false;

	if (encoding_ == DefaultEncoding) {
		bool forceUtf8 = false;
		if (f != stdin && UTF8 != Core::getSystemEncoding()) {
			long curpos = ftell(f);
			fseek(f, 0, SEEK_SET);
			unsigned char B[3];
			if (fread(B, 1, 3, f) == 3) {
				forceUtf8 = (B[0] == 0xEF && B[1] == 0xBB && B[2] == 0xBF);
			}
			fseek(f, curpos, SEEK_SET);
		}
		if (forceUtf8) {
			encoding_ = UTF8;
		} else {
			encoding_ = Core::getSystemEncoding();
		}
	}
	errStart_ = 0;
	errLength_ = 0;
	currentPosition_ = 0;

	if (f != stdin) {
		currentPosition_ = ftell(f);
	}
}

bool IO::InputStream::readRawChar(Char &x)
{
	if (type() == InternalBuffer) {
		if ((size_t) currentPosition_ == buffer_.length()) {
			return false;
		}
		x = buffer_.at(currentPosition_);
		currentPosition_ += 1;
		errLength_ += 1;
		return true;
	} else if (type() == ExternalBuffer) {
		return externalBuffer_->readRawChar(x);
	} else {
		if (lastCharHere_) {
			lastCharHere_ = false;
			currentPosition_ += lastCharLength_;
			x = lastChar_;
			return true;
		}
		lastCharLength_ = 0;
		char buf[4] = {0, 0, 0, 0};
		int ch = fgetc(file_);
		if (ch < 0) {
			return false;
		}
		buf[0] = ch;
		if (encoding_ != UTF8) {
			lastCharLength_ = 1;
			currentPosition_ +=  lastCharLength_;
		} else {
#if 0
			// More complex...
			long cpos = 1; //ftell(file_);
			fprintf(stderr, "cpos=%ld\n", cpos);
			if (cpos == 0) {
				// Try to read BOM
				static const char *BOM = "\xEF\xBB\xBF";
				char firstThree[3];
				bool seekBack = true;
				if (fread(firstThree, sizeof(char), 3, file_) == 3
					&& strncmp(BOM, firstThree, 3) == 0) {
					seekBack = false;
				}
				if (seekBack) {
					fseek(file_, 0, SEEK_SET);
				}
			}
			int ch = fgetc(file_);
			fprintf(stderr, "%s:%d ch=%d \n", __FILE__, __LINE__, ch);
			if (ch < 0) {
				return - 1;
			}
			lastCharBuffer_[0] = ch;
#endif

			uint8_t firstByte = buf[0];
			int extraBytes = 0;
			if (firstByte & 0x80) {
				if ((firstByte >> 5) == 0x06) {
					extraBytes = 1;
				} else if ((firstByte >> 4) == 0x0E) {
					extraBytes = 2;
				} else if ((firstByte >> 3) == 0x1E) {
					extraBytes = 3;
				}
			}
			bool isUtfSequence = true;
			for (int i = 0; i < extraBytes; i++) {
				ch = fgetc(file_);
				if (ch < 0 || (ch >> 6) != 2) {
					// Not valid UTF-8 follow byte! Unget read character and treat firstByte as single byte
					if (ch >= 0) {
						ungetc(ch, file_);
					}
					for (int j = i - 1; j >= 0; j--) {
						ungetc((unsigned char)buf[j + 1], file_);
					}
					isUtfSequence = false;
					break;
				}
				buf[i + 1] = ch;
			}
			if (isUtfSequence) {
				lastCharLength_ = 1 + extraBytes;
			} else {
				lastCharLength_ = 1;
			}
			currentPosition_ += lastCharLength_;
		}

		std::string sb(buf, lastCharLength_);
		EncodingError encodingError;
		std::wstring res;
		if (buf[0] == '\0' && lastCharLength_ == 1) {
			res = std::wstring(1, L'\0');
			encodingError = NoEncodingError;
		} else {
			res = Coder::decode(encoding_, sb, encodingError);
		}
		if (encodingError) {
			// Fallback for raw binary bytes: preserve exact byte value as character
			// Map byte to standard Kumir Char via StringUtils::symbol
			res = std::wstring(1, StringUtils::symbol(static_cast<unsigned char>(buf[0])));
			encodingError = NoEncodingError;
		}
		if (res.length() != 1) {
			Core::abort(Core::fromUtf8("Ошибка перекодирования при чтении данных из текстового файла"));
			return false;
		}
		x = lastChar_ = res.at(0);
		if (x == 0xfeff && currentPosition_ == lastCharLength_) {
			// Initial BOM, transform it to space
			x = lastChar_ = Char(' ');
		}
		return true;
	}
}


void IO::InputStream::pushLastCharBack()
{
	if (type() == InternalBuffer) {
		currentPosition_ -= 1;
		errLength_ -= 1;
	} else if (type() == ExternalBuffer) {
		externalBuffer_->pushLastCharBack();
	} else { /* File */

#if 0
		if (file_ == stdin) {
			if (lastCharBuffer_[2] != '\0') {
				ungetc(lastCharBuffer_[2], file_);
			}
			if (lastCharBuffer_[1] != '\0') {
				ungetc(lastCharBuffer_[1], file_);
			}
			int res = ungetc(lastCharBuffer_[0], file_);
			fprintf(stderr, "%s:%d ungetc=%d \n", __FILE__, __LINE__, res);
		} else {
			fseek(file_, -1 * strlen(lastCharBuffer_), SEEK_CUR);
		}
#else
		if (lastCharHere_) {
			fprintf(stderr, "InputStream: cannot push back more than one character, doing nothing.");
		} else {
			lastCharHere_ = true;
			currentPosition_ -= lastCharLength_;
		}
#endif
	}
}


String IO::InputStream::readUntil(const String &delimeters)
{
	String result;
	result.reserve(10);
	Char current;
	while (readRawChar(current)) {
		if (delimeters.find_first_of(current) != String::npos ) {
			pushLastCharBack();
			break;
		} else {
			result.push_back(current);
		}
	}
	return result;
}


void IO::InputStream::skipDelimiters(const String &del)
{
	const String &delim = del.empty() ? inputDelimiters : del;

	// Skip delimiters until lexem
	Char skip(32);
	while (readRawChar(skip)) {
		if (delim.find_first_of(skip) == String::npos) {
			pushLastCharBack();
			break;
		}
	}
	markPossibleErrorStart();
}


// Format parsing functions
StringList IO::splitIntoLexemsByDelimeter(
	const String &s,
	Char delim
) {
	StringList result;
	String current;
	current.reserve(10);
	for (size_t i = 0; i < s.length(); i++) {
		if (s[i] == delim) {
			result.push_back(current);
			current.clear();
		} else if (s[i] != ' ') {
			current.push_back(s[i]);
		}
	}
	if (current.length() > 0) {
		result.push_back(current);
	}
	return result;
}

String IO::readWord(InputStream &is)
{
	String delim = inputDelimiters;
	// Mark as lexem begin position
	is.skipDelimiters(delim);
	return is.readUntil(delim);
}


String IO::readString(InputStream &is)
{
	String delim = inputDelimiters;
	// Mark as lexem begin position
	is.skipDelimiters(delim);
	Char bracket = Char('\0');
	if (!is.readRawChar(bracket)) {
		is.setError(Core::fromUtf8("Не могу прочитать литерал: текст закончился"));
		return String();
	}
	if (bracket != Char('\'') && bracket != Char('"')) {
		is.pushLastCharBack();
		return is.readUntil(delim);
	}
	Char current;
	String result;
	result.reserve(10);
	while (is.readRawChar(current)) {
		if (current != bracket) {
			result.push_back(current);
		} else {
			break;
		}
	}
	if (current != bracket) {
//		is.setError(Core::fromUtf8("Ошибка чтения литерала: текст закончился раньше, чем появилась закрывающая кавычка"));
	}
	return result;
}

String IO::readLine(InputStream &is)
{
	String result;
	result.reserve(10);
	Char current;
	while (is.readRawChar(current)) {
		if (current == '\n') {
			break;
		}
		if (current != '\r') {
			result.push_back(current);
		}
	}

	return result;
}

Char IO::readChar(InputStream &is)
{
	Char result(0);
	if (is.hasError()) {
		return result;
	}

	bool ok = is.readRawChar(result);
	if (!ok) {
		is.setError(Core::fromUtf8("Ошибка ввода символа: текст закончился"));
	}

	return result;
}

int IO::readInteger(InputStream &is)
{
	String word = readWord(is);
	//fprintf(stderr, "%s:%d word='%S' \n", __FILE__, __LINE__, word.c_str());
	if (is.hasError()) {
		return 0;
	}
	Converter::ParseError error = Converter::NoError;
	int result = Converter::parseInt(word, 0, error);
	if (error == Converter::EmptyWord) {
		is.setError(Core::fromUtf8("Ошибка ввода целого числа: текст закончился"));
	} else if (error == Converter::BadSymbol) {
		is.setError(Core::fromUtf8("Ошибка ввода целого числа: число содержит неверный символ"));
	} else if (error == Converter::Overflow) {
		is.setError(Core::fromUtf8("Ошибка ввода: слишком большое целое число"));
	}
	return result;
}

real IO::readReal(InputStream &is)
{
	String word = readWord(is);
	if (is.hasError()) {
		return 0;
	}
	Converter::ParseError error = Converter::NoError;
	real result = Converter::parseReal(word, '.', error);
	if (error == Converter::EmptyWord) {
		is.setError(Core::fromUtf8("Ошибка ввода вещественного числа: текст закончился"));
	} else if (error == Converter::BadSymbol) {
		is.setError(Core::fromUtf8("Ошибка ввода вещественного числа: число содержит неверный символ"));
	} else if (error == Converter::WrongExpForm) {
		is.setError(Core::fromUtf8("Ошибка ввода вещественного числа: неверная запись экспоненциальной формы"));
	} else if (error == Converter::WrongReal) {
		is.setError(Core::fromUtf8("Ошибка ввода вещественного числа: неверная запись"));
	} else if (error == Converter::Overflow) {
		is.setError(Core::fromUtf8("Ошибка ввода: слишком большое вещественное число"));
	}
	return result;
}

static class BoolCode {
 public:
	bool value;
	const wchar_t *name;
} boolCodes[] = {
	{0, L"0"},
	{1, L"1"},
	{0, L"false"},
	{0, L"no"},
	{1, L"true"},
	{1, L"yes"},
	{1, L"да"},
	{1, L"истина"},
	{0, L"ложь"},
	{0, L"нет"},
}; // should be lexicographically ordered
static int boolCodesSize = sizeof(boolCodes) / sizeof(boolCodes[0]);

static bool operator <(const BoolCode &x, const BoolCode &y) {
	return String(x.name) < String(y.name);
}


bool IO::readBool(InputStream &is)
{
	String word = Core::toLowerCaseW(readWord(is));
	if (is.hasError()) {
		return 0;
	}
	if (word.length() == 0) {
		is.setError(Core::fromUtf8("Ошибка ввода логического значения: ничего не введено"));
	}

#if 0
	for (int i = 0; i < boolCodesSize; i++) {
		if (word == boolCodes[i].name) {
			return boolCodes[i].value;
		}
	}
#else
	BoolCode pat;
	pat.name = word.c_str();
	const BoolCode *first = boolCodes, *last = boolCodes + boolCodesSize;
	const BoolCode *here = std::lower_bound(first, last, pat);
	if (here != last && word == here->name) {
		return here->value;
	}
#endif

	is.setError(Core::fromUtf8("Ошибка ввода логического значения: неизвестное значение"));
	return false;
}

void IO::writeString(OutputStream &os, const String &str, int width)
{
	String data = str;
	if (width) {
		// TODO implement me
	}
	os.writeRawString(data);
}

void IO::writeChar(OutputStream &os, const Char &chr, int width)
{
	String data(1, chr);
	if (width) {
		// TODO implement me
	}
	os.writeRawString(data);
}

void IO::writeInteger(OutputStream &os, int value, int width)
{
	String strval = Converter::sprintfInt(value, 10, width, 'r');
	os.writeRawString(strval);
}

void IO::writeReal(OutputStream &os, real value, int width, int decimals)
{
	String strval = Converter::sprintfReal(value, '.', false, width, decimals, 'r');
	os.writeRawString(strval);
}

void IO::writeBool(OutputStream &os, bool value, int width)
{
	static const wchar_t *names[2] = {L"нет", L"да"};
//	static const String YES = Core::fromUtf8("да");
//	static const String NO = Core::fromUtf8("нет");
	String sval = names[value ? 1 : 0];
	if (width) {
		// TODO implement me
	}
	os.writeRawString(sval);
}


void IO::setLocaleEncoding(Encoding enc)
{
	LOCALE_ENCODING = enc;
}

IO::InputStream IO::makeInputStream(FileType fileNo, bool fromStdIn)
{
	if (fromStdIn && fileNo.getType() != FileType::Console) {
		return InputStream(Files::getAssignedIn(), LOCALE_ENCODING);
	} else if (fileNo.getType() == FileType::Console) {
		return InputStream(consoleInputBuffer);
	} else {
		std::deque<FileType>::iterator it = openedFiles.begin();
		for (; it != openedFiles.end(); ++it) {
			if (*it == fileNo) {
				break;
			}
		}
		if (it == openedFiles.end()) {
			Core::abort(Core::fromUtf8("Файл с таким ключем не открыт"));
			return InputStream();
		}
		if (it->getMode() != FileType::Read) {
			Core::abort(Core::fromUtf8("Файл с таким ключем открыт на запись"));
			return InputStream();
		}
		return InputStream(it->handle, fileEncoding);
	}
}

IO::OutputStream IO::makeOutputStream(FileType fileNo, bool toStdOut)
{
	if (toStdOut) {
		return OutputStream(Files::getAssignedOut(), LOCALE_ENCODING);
	} else if (fileNo.getType() == FileType::Console) {
		return OutputStream(consoleOutputBuffer);
	} else {
		std::deque<FileType>::iterator it = openedFiles.begin();
		for (; it != openedFiles.end(); ++it) {
			if (*it == fileNo) {
				break;
			}
		}
		if (it == openedFiles.end()) {
			Core::abort(Core::fromUtf8("Файл с таким ключем не открыт"));
			return OutputStream();
		}
		if (it->getMode() == FileType::Read) {
			Core::abort(Core::fromUtf8("Файл с таким ключем открыт на чтение"));
			return OutputStream();
		}
		return OutputStream(it->handle, fileEncoding);
	}
}


String IO::readString(FileType fileNo, bool fromStdIn)
{
	InputStream stream = makeInputStream(fileNo, fromStdIn);
	if (Core::getError().length() > 0) {
		return String();
	}
	return readString(stream);
}

String IO::readLine(FileType fileNo, bool fromStdIn)
{
	InputStream stream = makeInputStream(fileNo, fromStdIn);
	if (Core::getError().length() > 0) {
		return String();
	}
	return readLine(stream);
}

int IO::readInteger(FileType fileNo, bool fromStdIn)
{
	InputStream stream = makeInputStream(fileNo, fromStdIn);
	if (Core::getError().length() > 0) {
		return 0;
	}
	return readInteger(stream);
}

real IO::readReal(FileType fileNo, bool fromStdIn)
{
	InputStream stream = makeInputStream(fileNo, fromStdIn);
	if (Core::getError().length() > 0) {
		return 0.0;
	}
	return readReal(stream);
}

bool IO::readBool(FileType fileNo, bool fromStdIn)
{
	InputStream stream = makeInputStream(fileNo, fromStdIn);
	if (Core::getError().length() > 0) {
		return false;
	}
	return readBool(stream);
}

Char IO::readChar(FileType fileNo, bool fromStdIn)
{
	InputStream stream = makeInputStream(fileNo, fromStdIn);
	if (Core::getError().length() > 0) {
		return Char(' ');
	}
	return readChar(stream);
}


void IO::writeString(int width, const String &value, FileType fileNo, bool toStdOut)
{
	OutputStream stream = makeOutputStream(fileNo, toStdOut);
	if (Core::getError().length() > 0) {
		return;
	}
	writeString(stream, value, width);
}

void IO::writeChar(int width, Char value, FileType fileNo, bool toStdOut)
{
	OutputStream stream = makeOutputStream(fileNo, toStdOut);
	if (Core::getError().length() > 0) {
		return;
	}
	writeChar(stream, value, width);
}

void IO::writeInteger(int width, int value, FileType fileNo, bool toStdOut)
{
	OutputStream stream = makeOutputStream(fileNo, toStdOut);
	if (Core::getError().length() > 0) {
		return;
	}
	writeInteger(stream, value, width);
}

void IO::writeReal(int width, int decimals, real value, FileType fileNo, bool toStdOut)
{
	OutputStream stream = makeOutputStream(fileNo, toStdOut);
	if (Core::getError().length() > 0) {
		return;
	}
	writeReal(stream, value, width, decimals);
}

void IO::writeBool(int width, bool value, FileType fileNo, bool toStdOut)
{
	OutputStream stream = makeOutputStream(fileNo, toStdOut);
	if (Core::getError().length() > 0) {
		return;
	}
	writeBool(stream, value, width);
}


} // namespace Kumir
