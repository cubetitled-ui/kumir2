#include "kumfile.h"
#include <kumir2/analizer_sourcefileinterface.h>
#if QT_VERSION < 0x060000
#include <QRegExp>
#endif
#include <QStringList>
#include <QDebug>

//#if !defined(HAS_QCA) && defined(Q_OS_LINUX)
//#   define HAS_QCA
//#endif

#ifdef HAS_QCA
#   include <QtCrypto/qca.h>
#endif


Shared::Analizer::SFData KumFile::insertTeacherMark(
	Shared::Analizer::SFData &data
) { //Inserts |#%% if needed.

	#if QT_VERSION >= 0x060000
	static const QRegularExpression teacherMark("(^|\\n)\\|#%%");
#else
	static const QRegExp teacherMark("(^|\\n)\\|#%%");
#endif
	if (data.visibleText.indexOf(teacherMark) > -1) {
		//qDebug()<<" TM POS:"<<data.visibleText.indexOf("\n|#%%");
		return data;
	};
	if (data.hiddenText.indexOf(teacherMark) > -1) {
		// qDebug()<<" TM POS:"<<data.visibleText.indexOf("\n|#%%");
		return data;
	};
	// static const QRegExp speclAlg(QString::fromUtf8("^\\s*алг\\s*(\\S\\S\\S)?\\s+@"));
	#if QT_VERSION >= 0x060000
	static const QRegularExpression speclAlg(QString::fromUtf8("^|\\s*алг\\s*(\\S\\S\\S)?\\s*@"));
#else
	static const QRegExp speclAlg(QString::fromUtf8("^|\\s*алг\\s*(\\S\\S\\S)?\\s*@"));
#endif

	int pos = data.hiddenText.indexOf(speclAlg);
	qDebug() << data.hiddenText;
	if (pos > -1) {

		data.hiddenText.insert(pos, "\n|#%%\n");
		return data;
	};
	pos = data.visibleText.indexOf(speclAlg);
	if (pos > 1) {
		data.visibleText.insert(pos, "\n|#%%\n");
		QStringList strings = data.visibleText.split("\n", Qt::KeepEmptyParts);
		for (int i = 0; i < strings.count(); i++) {
			if (strings.at(i).startsWith("|#%%")) {
				data.protectedLineNumbers.insert(i);
				break;
			}
		}
		return data;
	}
	return data;
}


bool KumFile::hasCryptographicRoutines()
{
#ifdef HAS_QCA
	return true;
#else
	return false;
#endif
}

void KumFile::generateKeyPair(
	const QString &passPhrase,
	QString &privateKey,
	QString &publicKey
) {
	privateKey.clear();
	publicKey.clear();
#ifdef HAS_QCA
	QCA::PrivateKey seckey = QCA::KeyGenerator().createRSA(1024);
	QCA::SecureArray phrase(passPhrase.toUtf8());
	privateKey = seckey.toPEM(phrase);
	QCA::PublicKey pubkey = seckey.toPublicKey();
	publicKey = pubkey.toPEM();
#endif
}

void KumFile::signHiddenText(
	Shared::Analizer::SFData &data,
	const QString &privateKey,
	const QString &passPhrase
) {
#ifdef HAS_QCA
	QCA::SecureArray phrase(passPhrase.toUtf8());
	QCA::PrivateKey seckey = QCA::PrivateKey::fromPEM(privateKey, phrase);
	seckey.startSign(QCA::EMSA3_MD5);
	seckey.update(data.hiddenText.trimmed().toUtf8());
	data.hiddenTextSignature = seckey.signature();
#else
	data.hiddenTextSignature.clear();
#endif
}

KumFile::VerifyResult KumFile::verifyHiddenText(
	const Shared::Analizer::SFData &data,
	const QString &publicKey
) {
#ifdef HAS_QCA
	if (data.hiddenTextSignature.isEmpty()) {
		return NoSignature;
	}
	QCA::PublicKey pubkey = QCA::PublicKey::fromPEM(publicKey);
	pubkey.startVerify(QCA::EMSA3_MD5);
	pubkey.update(data.hiddenText.trimmed().toUtf8());
	return pubkey.validSignature(data.hiddenTextSignature)
		? SignatureMatch : SignatureMismatch;
#else
	return CryptographyNotSupported;
#endif
}
