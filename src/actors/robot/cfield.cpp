#include "cfield.h"
#include <assert.h>
#include <QDebug>
#include <QFile>
#include <QStringList>

namespace ActorRobot {

CFieldItem::CFieldItem()
{
	isColored = false;
	upChar    = QChar(' ');
	downChar  = QChar(' ');
	mark      = false;
	upWall    = false;
	downWall  = false;
	rightWall = false;
	leftWall  = false;
	radiation   = 0;
	temperature = 0;
}

void CFieldItem::setWalls(int wallByte)
{
	leftWall = (wallByte & LEFT_WALL) ? true : false;
	rightWall = (wallByte & RIGHT_WALL) ? true : false;
	downWall = (wallByte & DOWN_WALL) ? true : false;
	upWall = (wallByte & UP_WALL) ? true : false;
}

int CFieldItem::getWalls() const
{
	int res = 0;
	if (leftWall)
		res |= LEFT_WALL;
	if (rightWall)
		res |= RIGHT_WALL;
	if (downWall)
		res |= DOWN_WALL;
	if (upWall)
		res |= UP_WALL;
	return res;
}

bool CFieldItem::isEmpty(int wMask) const
{
	return
		(getWalls() & ~wMask) == 0 &&
		!isColored &&
		!mark &&
		upChar == ' ' &&
		downChar == ' ' &&
		radiation == 0 &&
		temperature == 0;
}


ConsoleField::ConsoleField(uint32_t rows, uint32_t cols)
{
	reset(rows, cols);
}

void  ConsoleField::reset(uint32_t rows, uint32_t cols)
{
	assert (0 < rows && 0 < cols);

	items.clear();
	items.resize(rows);

	for (uint32_t row = 0; row < rows; row++) {
		items[row].clear();
		items[row].resize(cols);
		for (uint32_t col = 0; col < cols; col++) {
			CFieldItem &it = items[row][col];
			it.upWall = (row == 0);
			it.downWall = (row + 1 == rows);
			it.leftWall = (col == 0);
			it.rightWall = (col + 1 == cols);
		}
	}

	roboRows = rows;
	roboCols = cols;
	roboRow = 0;
	roboCol = 0;
}


const CFieldItem *ConsoleField::getItem(uint32_t row, uint32_t col) const
{
	if (row >= roboRows  || col >= roboCols) {
		qDebug() << "ERROR!!! NO ITEM!!" << row << " " << col << " rows: " << roboRows << " cols: " << roboCols;
		return NULL;
	}

	return &items[row][col];
}

CFieldItem *ConsoleField::getItem(uint32_t row, uint32_t col)
{
	if (row >= roboRows  || col >= roboCols) {
		qDebug() << "ERROR!!! NO ITEM!!" << row << " " << col << " rows: " << roboRows << " cols: " << roboCols;
		return NULL;
	}

	return &items[row][col];
}

CFieldItem *ConsoleField::getCurItem()
{
	return getItem(roboRow, roboCol);
}


bool ConsoleField::isUpWall() const
{
	if (roboRow == 0)
		return true;
	const CFieldItem &itc = *getItem(roboRow, roboCol);
	const CFieldItem &itn = *getItem(roboRow - 1, roboCol);
	return itc.upWall || itn.downWall;
}

bool ConsoleField::isDownWall() const
{
	if (roboRow  + 1 == roboRows)
		return true;
	const CFieldItem &itc = *getItem(roboRow, roboCol);
	const CFieldItem &itn = *getItem(roboRow + 1, roboCol);
	return itc.downWall || itn.upWall;
}

bool ConsoleField::isLeftWall() const
{
	if (roboCol == 0)
		return true;
	const CFieldItem &itc = *getItem(roboRow, roboCol);
	const CFieldItem &itn = *getItem(roboRow, roboCol - 1);
	return itc.leftWall || itn.rightWall;
}

bool ConsoleField::isRightWall() const
{
	if (roboCol == 0)
		return true;
	const CFieldItem &itc = *getItem(roboRow, roboCol);
	const CFieldItem &itn = *getItem(roboRow, roboCol + 1);
	return itc.rightWall || itn.leftWall;
}


bool ConsoleField::goUp()
{
	if (isUpWall()) {
		return false;
	}
	roboRow--;
	return true;
}

bool ConsoleField::goDown()
{
	if (isDownWall()) {
		return false;
	}
	roboRow++;
	return true;
}

bool ConsoleField::goLeft()
{
	if (isLeftWall()) {
		return false;
	}
	roboCol--;
	return true;
}

bool ConsoleField::goRight()
{
	if (getItem(roboRow, roboCol)->rightWall) {
		return false;
	}
	roboCol++;
	return true;
}

int ConsoleField::saveToFile(const QString &filename) const
{
	QFile f(filename);
	bool ok = f.open(QIODevice::WriteOnly);
	if (!ok) {
		qDebug() << QString::fromUtf8("Ошибка сохранения обстановки ") << filename;
		return 1;
	}
	int res = saveToDataStream(f);
	f.close();
	return res;
}

int ConsoleField::createWallMask(uint32_t x, uint32_t y, uint32_t mx, uint32_t my)
{
	int res = 0;
	if (x == 0)
		res |= LEFT_WALL;
	if (y == 0)
		res |= UP_WALL;
	if (x + 1 == mx)
		res |= RIGHT_WALL;
	if (y + 1 == my)
		res |= DOWN_WALL;
	return res;
}

int ConsoleField::saveToDataStream(QIODevice &stream) const
{
	char ctmp[64];

	uint32_t mx = roboCols, my = roboRows;
	uint32_t rx = roboCol,  ry = roboRow;

	stream.write("; Field Size: x, y\n");
	sprintf(ctmp, "%u %u\n", mx, my);
	stream.write(ctmp);
	stream.write("; Robot position: x, y\n");
	sprintf(ctmp, "%u %u\n", rx, ry);
	stream.write(ctmp);
	stream.write("; A set of special Fields: x, y, Walls, Color, Radiation, Temperature, USymbol, DSymbol, Point\n");

	for (uint32_t y = 0; y < my; y++) {
		for (uint32_t x = 0; x < mx; x++) {
			int wMask = createWallMask(x, y, mx, my);

			CFieldItem item = items[y][x];
			if (item.isEmpty(wMask)) {
				continue;
			}
			char uChar = item.upChar.cell();
			if (uChar == ' ')
				uChar = '$';
			char dChar = item.downChar.cell();
			if (dChar == ' ')
				dChar = '$';
			char mark = item.mark ? '1' : '0';
			char color = item.isColored ? '1' : '0';
			sprintf(
				ctmp, "%d %d %d %c %f %f %c %c %c\n",
				x, y,
				item.getWalls() & ~wMask, color,
				item.radiation, item.temperature,
				uChar, dChar, mark
			);
			stream.write(ctmp);
		}
	}

	stream.write("; End Of File\n");

	return 0;
}

int ConsoleField::loadFromFile(const QString &filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly)) {
		qDebug() << QString::fromUtf8("Ошибка открытия обстановки!");
		return 1;
	}
	return loadFromDataStream(&f);
}

int ConsoleField::loadFromDataStream(QIODevice *stream)
{
	int NStrok = 0;

	int CurX, CurY;
	int SizeX, SizeY;

	// 1 stroka - razmery polya
	QString tmp = stream->readLine();

	if (tmp.isNull() || tmp.isEmpty()) {
		stream->close();
		return 2;
	}

	while (tmp.left(1) == ";" || tmp == "") {
		tmp = stream->readLine();
		NStrok++;
		if (tmp.isNull()) {
			stream->close();
			return 1;
		}
	}
	tmp = tmp.simplified();
	QStringList l_List = tmp.split(' ');

	if (l_List.count() != 2) {
		stream->close();
		return 3;
	}

	SizeX = l_List[0].toInt();
	SizeY = l_List[1].toInt();

	if (SizeX <= 0 || SizeY <= 0) {
		stream->close();
		return 4;
	}

	// Вторая строка - положение робота

	tmp = stream->readLine();

	if (tmp.isNull()) {
		stream->close();
		return 5;
	}

	while (tmp.left(1) == ";" || tmp == "") {
		tmp = stream->readLine();
		NStrok++;
		if (tmp.isNull()) {
			stream->close();
			return 5;
		}
	}
	tmp = tmp.simplified();
	l_List = tmp.split(' ');

	// koordinaty robota
	// proverka
	if ((l_List[0]).toInt() < 0 || (l_List[1]).toInt() < 0) {
		stream->close();
		return 6;
	}

	if ((l_List[0]).toInt() > SizeX || (l_List[1]).toInt() > SizeY) {
		stream->close();
		return 6;
	}

	while (!stream->atEnd()) {
		tmp = QString::fromUtf8(stream->readLine());
		NStrok++;
		if (tmp.isNull()) {

			return 5;
		}
		if (tmp.left(1) == ";" || tmp == "") {
			continue;
		}
		tmp = tmp.simplified();
		l_List = tmp.split(' ');
		if (l_List.count() == 0) {
			continue;
		}

		if (l_List.count() > 9) {
			stream->close();
			return -NStrok;
		}
		if (l_List.count() < 6) {
			stream->close();
			//    qDebug("N Lexem<6");
			return -NStrok;
		};
		bool ok;
		CurX = l_List[0].toInt(&ok);
		if (!ok) {
			stream->close();
			// qDebug("Bad cur X<6");
			return -NStrok;
		};

		CurY = l_List[1].toInt(&ok);

		if (!ok) {
			stream->close();
			qDebug("Bad curY <6");
			return -NStrok;
		};

		if (CurX < 0 || CurX > SizeX || CurY < 0 || CurY > SizeY) {

			stream->close();
			return -NStrok;
		}

		if (l_List[4].toFloat() < 0) {

			stream->close();
			return -NStrok;
		}


		if (l_List[5].toFloat() < MIN_TEMP) {

			stream->close();
			return -NStrok;
		}


		if (l_List.count() >= 7) {

			QString tmp1 = l_List[6];
			//dlina lexemy dolzna ravnyatsa 1
			if (!(tmp1.length() == 1)) {

				stream->close();
				return -NStrok;
			}


		}


		if (l_List.count() >= 8) {

			QString tmp1 = l_List[7];
			//dlina lexemy dolzna ravnyatsa 1
			if (!(tmp1.length() == 1)) {
				stream->close();
				return -NStrok;
			}
		}
	}
	stream->close();

	if (!stream->open(QIODevice::ReadOnly)) {

		return 10;
	}


	// 1 stroka - razmery polya
	tmp = stream->readLine();

	if (tmp.isNull()) {
		stream->close();

		return 10;
	}

	while (tmp.left(1) == ";" || tmp == "") {
		tmp = QString::fromUtf8(stream->readLine());
		NStrok++;
		if (tmp.isNull()) {
			stream->close();

			return 10;
		}
	}
	tmp = tmp.simplified();
	l_List = tmp.split(' ');

	if (l_List.count() != 2) {
		stream->close();

		return -NStrok;
	}

	SizeX = (l_List[1]).toInt();
	SizeY = (l_List[0]).toInt();
	//   //NEW ROBO Field
	reset(SizeX, SizeY);
	qDebug() << "FIELD SIZE ROW" << SizeX << " COL:" << SizeY;


	if ((l_List[0]).toInt() <= 0 || (l_List[1]).toInt() <= 0) {

		stream->close();
		return - NStrok;
	}


	// Вторая строка - положение робота

	tmp = stream->readLine();


	if (tmp.isNull()) {
		stream->close();

		return 10;
	}

	while (tmp.left(1) == ";" || tmp == "") {
		tmp = stream->readLine();
		NStrok++;
		if (tmp.isNull()) {
			stream->close();

			return 10;
		}
	}
	tmp = tmp.simplified();
	l_List = tmp.split(' ');

	// koordinaty robota

	if ((l_List[0]).toInt() < 0 || (l_List[1]).toInt() < 0) {

		stream->close();
		return - NStrok;
	}

	if ((l_List[0]).toInt() > SizeY || (l_List[1]).toInt() > SizeX) {

		stream->close();
		return - NStrok;
	}

	roboCol = (l_List[0]).toInt();
	roboRow = (l_List[1]).toInt();
	qDebug() << "ROBOT POS ROW" << roboRow << " COL:" << roboCol;

	while (!stream->atEnd()) {
		tmp = QString::fromUtf8(stream->readLine());
		NStrok++;
		if (tmp.isNull()) {

			stream->close();
			return 10;
		}
		if (tmp.left(1) == ";" || tmp == "") {
			continue;
		}
		tmp = tmp.simplified();
		l_List = tmp.split(' ');
		if (l_List.count() == 0) {
			continue;
		}

		if (l_List.count() > 9) {

			stream->close();
			return -NStrok;
		}
		CurX = l_List[1].toInt();
		CurY = l_List[0].toInt();
		if (CurX < 0 || CurX > SizeX || CurY < 0 || CurY > SizeY) {
			qDebug() << "Out of field";
			stream->close();
			return -NStrok;
		}
		// TODO STENI
		// qDebug()<<"Before Walls";
		if (getItem(CurX, CurY)) {
			getItem(CurX, CurY)->setWalls((l_List[2]).toInt());
			if (CurX == 0) {
				getItem(CurX, CurY)->upWall = true;
			}
			if (CurX == SizeX - 1) {
				getItem(CurX, CurY)->downWall = true;
			}
			if (CurY == 0) {
				getItem(CurX, CurY)->leftWall = true;
			}
			if (CurY == SizeY - 1) {
				getItem(CurX, CurY)->rightWall = true;
			}
		}

		//   qDebug()<<"After Walls";

		if (!((l_List[3]).toInt() == 0)) {
			getItem(CurX, CurY)->isColored = true;
		} else {
			if (getItem(CurX, CurY)) {
				getItem(CurX, CurY)->isColored = false;
			}
		}
		qreal radiation = (l_List[4].replace(",", ".")).toDouble();
		if (getItem(CurX, CurY)) {
			getItem(CurX, CurY)->radiation = radiation;
		}

		if (l_List[4].toFloat() < 0) {

			stream->close();
			return -NStrok;
		}
		qreal temperature = (l_List[5].replace(",", ".")).toDouble();
		if (getItem(CurX, CurY)) {
			getItem(CurX, CurY)->temperature = temperature;
		}

		if (l_List[5].toFloat() < MIN_TEMP) {

			stream->close();
			return -NStrok;
		}

		if (l_List.count() >= 7) {

			QString tmp1 = l_List[6];
			//dlina lexemy dolzna ravnyatsa 1
			if (!(tmp1.length() == 1)) {

				stream->close();
				return -NStrok;
			}
			//qDebug()<<QString::fromUtf8("Тест Up:")<<tmp1[0];
			if (tmp1[0] != '$') {
				if (getItem(CurX, CurY)) {
					getItem(CurX, CurY)->upChar = tmp1[0];
				}
			} else {
				if (getItem(CurX, CurY)) {
					getItem(CurX, CurY)->upChar = ' ' ;
				}
			}
		} else {
			if (getItem(CurX, CurY)) {
				getItem(CurX, CurY)->upChar = ' ' ;
			}
		}

		if (l_List.count() >= 8) {

			QString tmp1 = l_List[7];

			//dlina lexemy dolzna ravnyatsa 1
			if (!(tmp1.length() == 1)) {

				stream->close();
				return -NStrok;
			}
			//qDebug()<<QString::fromUtf8("Тест Down:")<<tmp1[0];
			if (tmp1[0] != '$') {
				if (getItem(CurX, CurY)) {
					getItem(CurX, CurY)->downChar = tmp1[0];
				}
			} else {
				if (getItem(CurX, CurY)) {
					getItem(CurX, CurY)->downChar = ' ' ;
				}
			}
		} else {
			if (getItem(CurX, CurY)) {
				getItem(CurX, CurY)->downChar = ' ' ;
			}
		}

		if (l_List.count() >= 9) {

			QString tmp1 = l_List[8];
			//dlina lexemy dolzna ravnyatsa 1
			if (!(tmp1.length() == 1)) {

				stream->close();
				return -NStrok;
			}
			if (tmp1[0] == '1') {
				if (getItem(CurX, CurY)) {
					getItem(CurX, CurY)->mark = true;
				}
			} else {
				if (getItem(CurX, CurY)) {
					getItem(CurX, CurY)->mark = false ;
				}
			}
		} else {
			if (getItem(CurX, CurY)) {
				getItem(CurX, CurY)->mark = false ;
			}
		}
	}

	stream->close();
	return 0;
}

} // ActorRobot namespace

