#include "imagebutton.h"

#include <QMouseEvent>
#include <QPixmap>

ImageButton::ImageButton(QWidget *parent) : QLabel(parent) {
    setScaledContents(true);
    setStyleSheet(QStringLiteral("ImageButton { background: transparent; border: none; }") + toolTipStyleSheet());
}

// Подсказка наследует таблицы стилей родителей, поэтому правило задаётся в собственном стиле виджета.
QString ImageButton::toolTipStyleSheet() {
    return QStringLiteral(
        "QToolTip {"
        "  color: #575757;"
        "  background-color: #ffffff;"
        "  border: 1px solid #767676;"
        "  padding: -1px -1px 0px -1px;"
        "  font-family: 'Segoe UI';"
        "  font-size: 9pt;"
        "}"
    );
}

void ImageButton::setImagePath(const QString &path) {
    setPixmap(QPixmap(path));
}

void ImageButton::mousePressEvent(QMouseEvent *event) {
    if (event && event->button() == Qt::LeftButton) {
        emit clicked();
        event->accept();
        return;
    }
    QLabel::mousePressEvent(event);
}
