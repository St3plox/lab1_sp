#pragma once

#include <QLineEdit>

// 9 = ASCII digit, a = Latin letter, - = literal minus.
class MaskedLineEdit final : public QLineEdit {
public:
    explicit MaskedLineEdit(const QString &mask, QWidget *parent = nullptr);

    QString mask() const { return mask_; }
    bool isComplete() const;
    QString value() const;

private:
    QString mask_;
};
