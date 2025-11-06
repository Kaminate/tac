#include "builder.h"

#include "atom/atom_basic.h"
#include "box/box_group.h"
#include "box/box_single.h"
#include "core/formula.h"
#include "core/split.h"
#include "utils/exceptions.h"

using namespace microtex;

RenderBuilder& RenderBuilder::setLineSpace(const Dimen& dimen) {
  if (!dimen.isValid()) {
    MicroTexError("Cannot set line space without having specified a width!");
    return *this;
  }
  _lineSpace = dimen;
  return *this;
}

Render* RenderBuilder::build(Formula& f) {
  return build(f._root);
}

Render* RenderBuilder::build(const sptr<Atom>& fc) {
  sptr<Atom> f = fc;
  if (f == nullptr) f = sptrOf<EmptyAtom>();
  if (_textSize == -1) {
    MICROTEX_ERROR_RET("A text size is required, call function setTextSize before build.");
  }
  if (_mathFontName.empty()) {
    MICROTEX_ERROR_RET("A math font is required, call function setMathFontName before build.");
  }

  auto fctx = sptrOf<FontContext>();
  fctx->selectMathFont(_mathFontName);
  fctx->selectMainFont(_mainFontName);

  const auto style = _style;
  Env env(style, fctx, _textSize);
  const auto isLimitedWidth = !_textWidth.isEmpty();
  if (isLimitedWidth) {
    env.setTextWidth(_textWidth);
  }
  if (_lineSpace.isValid()) {
    env.setLineSpace(_lineSpace);
  }

  Render* render;
  auto box = f->createBox(env);
  if (isLimitedWidth) {
    HBox* hb;
    bool isBoxSplit = false;
    if (!_lineSpace.isEmpty()) {
      auto space = Units::fsize(_lineSpace, env);
      auto [isSplit, split] = BoxSplitter::split(box, env.textWidth(), space);
      isBoxSplit = isSplit;
      hb = new HBox(split, _fillWidth ? env.textWidth() : split->_width, _align);
    } else {
      hb = new HBox(box, _fillWidth ? env.textWidth() : box->_width, _align);
    }
    render = new Render(sptr<Box>(hb), _textSize, isBoxSplit);
  } else {
    render = new Render(box, _textSize);
  }

  if (!isTransparent(_fg)) render->setForeground(_fg);
  return render;
}
