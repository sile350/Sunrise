#ifndef STORIESENGINE_H
#define STORIESENGINE_H

#include "exengine.h"

// Порт Engines\stories.cs: упражнение 2.2.2 «Пересказ рассказа и ответы на вопросы».
class StoriesEngine : public ExEngine {
    Q_OBJECT
public:
    explicit StoriesEngine(QObject *parent = nullptr) : ExEngine(parent) {}

    int storyIndex = 1;      // 1..N
    bool contentVisible = false; // «Показать/Скрыть на экране пациента» (stories.Designer: Visible = false)

    void rebuild();
    int storyCount() const;
    QString storyTitle(int index) const;

private:
    QStringList tokens() const;
};

#endif
