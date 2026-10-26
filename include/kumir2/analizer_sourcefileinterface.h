#ifndef ANALIZER_SOURCEFILEINTERFACE_H
#define ANALIZER_SOURCEFILEINTERFACE_H

#include <QtPlugin>
#include <QSet>
#include <QUrl>
#include <QTextStream>
#include <QIODevice>
#include <kumir2_qt6_compat.h>

namespace Shared
{

namespace Analizer
{

struct SFData {
	QString visibleText;
	QSet<int> protectedLineNumbers;
	QString hiddenText;
	bool hasHiddenText;
	QString sourceEncoding;
	QByteArray hiddenTextSignature;
	QString canonicalSourceLanguageName; // the same as file suffix after '.'
	QUrl sourceUrl;
};

class SourceFileInterface
{
public:
	typedef Shared::Analizer::SFData Data;

public /*methods*/:

	// Abstract methods to read and write from/to string in language-specific way

	virtual QString toString(const SFData &data) const = 0;
	virtual SFData fromString(const QString &string) const = 0;

	// Generic methods to read and write from/to raw byte array. Default implementation
	// assumes UTF-8 encoding with BOM mark, but it is possible to override

	virtual QByteArray toBytes(
		const SFData &data,
		const QString &forceEncoding = ""
	) const {
		QByteArray buffer;
		QTextStream stream(&buffer, QIODevice::WriteOnly);
		if (forceEncoding.length() > 0) {
			setTextStreamCodec(stream, forceEncoding);
		} else {
			setTextStreamCodec(stream, "UTF-8");
			stream.setGenerateByteOrderMark(true);
		}
		stream << toString(data);
		stream.flush();
		return buffer;
	}

	virtual SFData fromBytes(
		const QByteArray &bytes,
		const QString &forceEncoding = ""
	) const {
		QByteArray bytesCopy(bytes);
		QTextStream stream(&bytesCopy);
		if (forceEncoding.length() > 0) {
			setTextStreamCodec(stream, forceEncoding);
		} else {
			setTextStreamCodec(stream, "UTF-8");
			stream.setAutoDetectUnicode(true);
		}
		QString string = stream.readAll();
		return fromString(string);
	}

};

} // namespace Analizer

} // namespace Shared

Q_DECLARE_INTERFACE(
	Shared::Analizer::SourceFileInterface,
	"kumir2.Analizer.SourceFileInterface"
)

#endif // ANALIZER_SOURCEFILEINTERFACE_H
