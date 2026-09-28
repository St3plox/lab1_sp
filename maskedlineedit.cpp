#include "maskedlineedit.h"

#include <QValidator>

#include <stdexcept>

namespace {
class MaskValidator final : public QValidator {
public:
  explicit MaskValidator(QString mask, QObject *parent)
      : QValidator(parent), mask_(std::move(mask)) {}

  State validate(QString &input, int &position) const override {
    Q_UNUSED(position)
    if (input.size() > mask_.size())
      return Invalid;

    bool complete = input.size() == mask_.size();
    for (qsizetype i = 0; i < input.size(); ++i) {
      const QChar actual = input[i];
      if (actual == QLatin1Char('_') && mask_[i] != QLatin1Char('-')) {
        complete = false;
        continue;
      }

      bool matches = false;
      switch (mask_[i].toLatin1()) {
      case '9':
        matches = actual >= QLatin1Char('0') && actual <= QLatin1Char('9');
        break;
      case 'a':
        matches = (actual >= QLatin1Char('A') && actual <= QLatin1Char('Z')) ||
                  (actual >= QLatin1Char('a') && actual <= QLatin1Char('z'));
        break;
      case '-':
        matches = actual == QLatin1Char('-');
        break;
      }
      if (!matches)
        return Invalid;
    }
    return complete ? Acceptable : Intermediate;
  }

private:
  QString mask_;
};
} // namespace

MaskedLineEdit::MaskedLineEdit(const QString &mask, QWidget *parent)
    : QLineEdit(parent), mask_(mask) {
  if (mask_.isEmpty())
    throw std::invalid_argument("Mask must not be empty");

  QString qtMask;
  for (const QChar ch : mask_) {
    if (ch == QLatin1Char('9'))
      qtMask += QLatin1Char('0'); // required digit
    else if (ch == QLatin1Char('a'))
      qtMask += QLatin1Char('A'); // required letter
    else if (ch == QLatin1Char('-'))
      qtMask += QStringLiteral("\\-"); // literal minus
    else
      throw std::invalid_argument("Mask may contain only 9, a, and -");
  }
  qtMask += QStringLiteral(";_");

  setInputMask(qtMask);
  setValidator(new MaskValidator(mask_, this));
  setClearButtonEnabled(true);
}

bool MaskedLineEdit::isComplete() const { return hasAcceptableInput(); }

QString MaskedLineEdit::value() const {
  return isComplete() ? text() : QString();
}
