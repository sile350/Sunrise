#include "storiesengine.h"

#include <QPainter>
#include <QTextDocument>

namespace {

QPixmap renderText(const QString &text, int width, int pointSize, bool bold, bool center) {
    QTextDocument doc;
    QFont f(QStringLiteral("Arial"));
    f.setPixelSize(qRound(pointSize * 96.0 / 72.0));
    f.setBold(bold);
    doc.setDefaultFont(f);
    QTextOption opt;
    opt.setWrapMode(QTextOption::WordWrap);
    opt.setAlignment(center ? Qt::AlignHCenter : Qt::AlignLeft);
    doc.setDefaultTextOption(opt);
    doc.setTextWidth(width);
    QString plain = text;
    plain.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    plain = plain.trimmed();
    // Label переносит строку на каждом \r\n из data.txt
    doc.setPlainText(plain);
    const int h = qMax(1, int(doc.size().height()) + 4);
    QPixmap pm(width, h);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setPen(Qt::black);
    doc.drawContents(&p);
    p.end();
    return pm;
}

} // namespace

QStringList StoriesEngine::tokens() const {
    const QString text = ExAssets::readAll(ExAssets::exnPath(QStringLiteral("2.2.2/data.txt")));
    return text.split(QLatin1Char('|'));
}

int StoriesEngine::storyCount() const {
    return qMax(1, tokens().size() / 3);
}

QString StoriesEngine::storyTitle(int index) const {
    const QStringList t = tokens();
    const int i = (qBound(1, index, qMax(1, t.size() / 3)) - 1) * 3;
    return i < t.size() ? t[i].trimmed() : QString();
}

void StoriesEngine::rebuild() {
    clear();
    block = true;
    background = QColor(249, 249, 249);
    const QStringList t = tokens();
    if (t.size() < 3) {
        emit changed();
        return;
    }
    const int rec = qBound(1, storyIndex, qMax(1, t.size() / 3)) - 1;
    const QString title = t[rec * 3].trimmed();
    const QString body = t[rec * 3 + 1].trimmed();
    const QString questions = t[rec * 3 + 2].trimmed();
    // uprSpace: stories.Top=100, Left=550; контролы по stories.Designer.cs / initEx:
    // lzag (297,53) 16pt; ltext (0->242?,104) ширина 800, Arial 14; label2 (12,424) 12pt bold;
    // lq: Top = ltext.Top + ltext.Height(280) + 15, Left 150, Width 600, Arial 14.
    const int hostX = 550;
    const int hostY = 100;
    auto add = [&](const QPixmap &pm, int x, int y, const QString &name) {
        const int i = addSprite();
        sprites[i].pix = pm;
        sprites[i].w = pm.width();
        sprites[i].h = pm.height();
        sprites[i].x = hostX + x;
        sprites[i].y = hostY + y;
        sprites[i].name = name;
        // у врача (свой экземпляр stories) текст виден всегда; у пациента — после «Показать на экране пациента»
        sprites[i].doctorOnly = !contentVisible;
    };
    add(renderText(title, 1000, 16, false, false), 297, 53, QStringLiteral("zag"));
    add(renderText(body, 800, 14, false, false), 0, 104, QStringLiteral("text"));
    add(renderText(QStringLiteral("Вопросы:"), 300, 12, true, false), 12, 424, QStringLiteral("label2"));
    add(renderText(questions, 600, 14, false, false), 150, 104 + 280 + 15, QStringLiteral("questions"));
    emit changed();
}
