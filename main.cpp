#include "maskedlineedit.h"

#include <QApplication>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle(QStringLiteral("Ввод по маске — вариант 3"));
    window.setMinimumWidth(430);

    auto *layout = new QVBoxLayout(&window);
    auto *title = new QLabel(QStringLiteral("Поле ввода по маске"));
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);
    layout->addWidget(new QLabel(QStringLiteral("9 — цифра, a — латинская буква, - — знак минус.")));

    auto *form = new QFormLayout;
    auto *digits = new MaskedLineEdit(QStringLiteral("999"));
    auto *code = new MaskedLineEdit(QStringLiteral("aaa-999"));
    auto *mixed = new MaskedLineEdit(QStringLiteral("99-aa"));
    form->addRow(QStringLiteral("999:"), digits);
    form->addRow(QStringLiteral("aaa-999:"), code);
    form->addRow(QStringLiteral("99-aa:"), mixed);
    layout->addLayout(form);

    auto *status = new QLabel;
    status->setWordWrap(true);
    layout->addWidget(status);

    const auto updateStatus = [=] {
        QStringList ready;
        for (auto *field : {digits, code, mixed}) {
            if (field->isComplete())
                ready << field->mask() + QStringLiteral(": ") + field->value();
        }
        status->setText(ready.isEmpty()
            ? QStringLiteral("Заполните поля. Неверные символы не вводятся.")
            : QStringLiteral("Корректные значения: ") + ready.join(QStringLiteral("; ")));
    };
    for (auto *field : {digits, code, mixed})
        QObject::connect(field, &QLineEdit::textChanged, &window, updateStatus);
    updateStatus();

    auto *clear = new QPushButton(QStringLiteral("Очистить поля"));
    QObject::connect(clear, &QPushButton::clicked, &window, [=] {
        for (auto *field : {digits, code, mixed})
            field->clear();
        digits->setFocus();
        updateStatus();
    });
    layout->addWidget(clear);

    window.show();
    return app.exec();
}
