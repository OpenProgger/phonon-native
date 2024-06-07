module;

#include <QMediaPlayer>
#include <QQuickItem>
#include <QQuickView>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoSink>
#include <QtCore/qtmochelpers.h>
#include <phonon/VideoWidgetInterface>

#define MAX_COLOR_VALUE 255

export module phonon_native:videowidget;

import :sinknode;

export namespace Phonon::Native {
	class VideoWidget final:
		public QWidget,
		public VideoWidgetInterface44,
		public SinkNode {
		Q_OBJECT
		Q_INTERFACES(Phonon::VideoWidgetInterface44)

	  public:
		explicit VideoWidget(QWidget* parent):
			QWidget{parent, Qt::WindowFlags()} {
			auto* layout{new QVBoxLayout(this)};
			auto* view{new QQuickView{QUrl::fromLocalFile(":/video.qml"),
				static_cast<QWindow*>(nullptr)}};
			view->setResizeMode(QQuickView::SizeRootObjectToView);
			auto* container{
				QWidget::createWindowContainer(view, this, Qt::WindowFlags())};
			container->setMinimumSize(view->size());
			container->setFocusPolicy(Qt::TabFocus);
			layout->addWidget(container, 0, Qt::Alignment());
			m_sink = qvariant_cast<QVideoSink*>(
				view->rootObject()->children().first()->property("videoSink"));
			m_effect = view->rootObject()->children()[1];
		}

		~VideoWidget() final = default;
		VideoWidget(const VideoWidget&) = delete;
		VideoWidget(VideoWidget&&) = delete;
		auto operator=(const VideoWidget&) -> VideoWidget& = delete;
		auto operator=(VideoWidget&&) -> VideoWidget& = delete;

		[[nodiscard]]
		auto aspectRatio() const -> Phonon::VideoWidget::AspectRatio final {
			return m_aspect;
		}

		auto setAspectRatio(
			Phonon::VideoWidget::AspectRatio ratio) -> void final {
			m_aspect = ratio;
		}

		[[nodiscard]]
		auto brightness() const -> qreal final {
			return m_brightness;
		}

		auto setBrightness(qreal brightness) -> void final {
			m_brightness = brightness;
			m_effect->setProperty("brightness", m_brightness);
		}

		[[nodiscard]]
		auto scaleMode() const -> Phonon::VideoWidget::ScaleMode final {
			return m_scale;
		}

		auto setScaleMode(Phonon::VideoWidget::ScaleMode mode) -> void final {
			m_scale = mode;
			m_effect->setProperty("fillMode",
				mode == Phonon::VideoWidget::FitInView
					? Qt::KeepAspectRatio
					: Qt::KeepAspectRatioByExpanding);
		}

		[[nodiscard]]
		auto contrast() const -> qreal final {
			return m_contrast;
		}

		auto setContrast(qreal contrast) -> void final {
			m_contrast = contrast;
			m_effect->setProperty("contrast", m_contrast);
		}

		[[nodiscard]]
		auto hue() const -> qreal final {
			return m_hue;
		}

		auto setHue(qreal hue) -> void final {
			m_hue = hue;
			if(m_hue > 0) {
				m_effect->setProperty("colorization", m_hue);
				m_effect->setProperty("colorizationColor",
					QColor::fromHsv(MAX_COLOR_VALUE,
						MAX_COLOR_VALUE,
						MAX_COLOR_VALUE,
						MAX_COLOR_VALUE));
			} else if(m_hue < 0) {
				m_effect->setProperty("colorization", qFabs(m_hue));
				m_effect->setProperty("colorizationColor",
					QColor::fromHsv(
						0, MAX_COLOR_VALUE, MAX_COLOR_VALUE, MAX_COLOR_VALUE));
			} else {
				m_effect->setProperty("colorization", m_hue);
			}
		}

		[[nodiscard]]
		auto saturation() const -> qreal final {
			return m_saturation;
		}

		auto setSaturation(qreal saturation) -> void final {
			m_saturation = saturation;
			m_effect->setProperty("saturation", m_saturation);
		}

		auto widget() -> QWidget* final {
			return this;
		}

		[[nodiscard]]
		auto snapshot() const -> QImage final {
			return m_sink->videoFrame().toImage();
		}

		auto connectToMediaPlayer(QMediaPlayer* player) -> void final {
			player->setVideoOutput(m_sink);
			SinkNode::connectToMediaPlayer(player);
		}

		auto disconnectFromMediaPlayer(QMediaPlayer* player) -> void final {
			player->setVideoOutput(nullptr);
			SinkNode::disconnectFromMediaPlayer(player);
		}

	  private:
		QVideoSink* m_sink;
		QObject* m_effect;
		qreal m_hue{};
		qreal m_saturation{};
		qreal m_brightness{};
		qreal m_contrast{};
		Phonon::VideoWidget::ScaleMode m_scale{};
		Phonon::VideoWidget::AspectRatio m_aspect{};
	};
} // namespace Phonon::Native

#include "videowidget.moc"
