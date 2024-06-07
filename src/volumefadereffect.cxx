module;

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QTimeLine>
#include <QtCore/qtmochelpers.h>
#include <phonon/VolumeFaderInterface>

export module phonon_native:volumefadereffect;

import :sinknode;

export namespace Phonon::Native {
	class VolumeFaderEffect final:
		public QObject,
		public VolumeFaderInterface,
		public SinkNode {
		Q_OBJECT
		Q_INTERFACES(Phonon::VolumeFaderInterface)

	  private slots:

		auto slotSetVolume(qreal volume) -> void {
			m_output->setVolume(
				m_fadeFromVolume
				+ (static_cast<float>(volume)
					* (static_cast<float>(m_fadeToVolume) - m_fadeFromVolume)));
		}

	  public:
		explicit VolumeFaderEffect(QObject* parent):
			QObject{parent}, m_fadeTimeline{new QTimeLine{1000, this}} {
			connect(m_fadeTimeline,
				&QTimeLine::valueChanged,
				this,
				&VolumeFaderEffect::slotSetVolume,
				Qt::AutoConnection);
		}

		~VolumeFaderEffect() final = default;
		VolumeFaderEffect(const VolumeFaderEffect&) = delete;
		VolumeFaderEffect(VolumeFaderEffect&&) = delete;
		auto operator=(const VolumeFaderEffect&) -> VolumeFaderEffect& = delete;
		auto operator=(VolumeFaderEffect&&) -> VolumeFaderEffect& = delete;

		virtual float volume() const final {
			return m_output->volume();
		}

		virtual void setVolume(float volume) final {
			m_fadeTimeline->stop();
			m_output->setVolume(volume);
		}

		virtual Phonon::VolumeFaderEffect::FadeCurve fadeCurve() const final {
			return m_fadeCurve;
		}

		virtual void setFadeCurve(
			Phonon::VolumeFaderEffect::FadeCurve pFadeCurve) final {
			m_fadeCurve = pFadeCurve;
			QEasingCurve fadeCurve;
			switch(pFadeCurve) {
				case Phonon::VolumeFaderEffect::Fade3Decibel:
					fadeCurve = QEasingCurve::InQuad;
					break;
				case Phonon::VolumeFaderEffect::Fade6Decibel:
					fadeCurve = QEasingCurve::Linear;
					break;
				case Phonon::VolumeFaderEffect::Fade9Decibel:
					fadeCurve = QEasingCurve::OutCubic;
					break;
				case Phonon::VolumeFaderEffect::Fade12Decibel:
					fadeCurve = QEasingCurve::OutQuart;
					break;
			}
			m_fadeTimeline->setEasingCurve(fadeCurve);
		}

		virtual void fadeTo(float targetVolume, int fadeTime) final {
			m_fadeTimeline->stop();
			m_fadeToVolume = static_cast<qreal>(targetVolume);
			m_fadeFromVolume = m_output->volume();

			if(fadeTime <= 0) {
				setVolume(targetVolume);
				return;
			}

			m_fadeTimeline->setDuration(fadeTime);
			m_fadeTimeline->start();
		}

		auto connectToMediaPlayer(QMediaPlayer* player) -> void final {
			m_output = player->audioOutput();
			SinkNode::connectToMediaPlayer(player);
		}

		auto disconnectFromMediaPlayer(QMediaPlayer* player) -> void final {
			m_output = nullptr;
			SinkNode::disconnectFromMediaPlayer(player);
		}

	  private:
		QAudioOutput* m_output;
		QTimeLine* m_fadeTimeline;
		qreal m_fadeToVolume;
		float m_fadeFromVolume;
		Phonon::VolumeFaderEffect::FadeCurve m_fadeCurve;
	};
} // namespace Phonon::Native

#include "volumefadereffect.moc"
