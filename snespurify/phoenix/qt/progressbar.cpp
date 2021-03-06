void ProgressBar::create(Window &parent, unsigned x, unsigned y, unsigned width, unsigned height) {
  progressBar->setParent(parent.window->container);
  progressBar->setGeometry(x, y, width, height);
  progressBar->setRange(0, 100);
  progressBar->setTextVisible(false);
  progressBar->show();
}

void ProgressBar::setPosition(unsigned position) {
  progressBar->setValue(position);
}

ProgressBar::ProgressBar() {
  progressBar = new ProgressBar::Data(*this);
  widget->widget = progressBar;
}
