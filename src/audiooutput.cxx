module;

#include <QAudioDevice>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QtCore/qtmochelpers.h>
#include <phonon/AudioOutputInterface>

export module phonon_native:audiooutput;

import :sinknode;

export namespace Phonon::Native {
	class AudioOutput final:
		public QObject,
		public AudioOutputInterface,
		public SinkNode {
		Q_OBJECT
		Q_INTERFACES(Phonon::AudioOutputInterface)

	  public:
		explicit AudioOutput(QObject* parent):
			QObject{parent}, m_output{new QAudioOutput{this}} {
			connect(m_output,
				&QAudioOutput::mutedChanged,
				this,
				&AudioOutput::mutedChanged,
				Qt::AutoConnection);
			connect(m_output,
				&QAudioOutput::volumeChanged,
				this,
				&AudioOutput::volumeChanged,
				Qt::AutoConnection);
		}

		~AudioOutput() final = default;
		AudioOutput(const AudioOutput&) = delete;
		AudioOutput(AudioOutput&&) = delete;
		auto operator=(const AudioOutput&) -> AudioOutput& = delete;
		auto operator=(AudioOutput&&) -> AudioOutput& = delete;

		[[nodiscard]]
		auto volume() const -> qreal final {
			return static_cast<qreal>(m_output->volume());
		}

		auto setVolume(qreal volume) -> void final {
			m_output->setVolume(static_cast<float>(volume));
		}

		[[nodiscard]]
		auto outputDevice() const -> int final {
			auto devices{QMediaDevices::audioOutputs()};
			for(auto i{0}; i < devices.size(); i++) {
				if(devices[i].id().compare(
					   m_output->device().id(), Qt::CaseSensitive)
					== 0) {
					return i;
				}
			}
			return {};
		}

		[[nodiscard]]
		auto setOutputDevice(int index) -> bool final {
			m_output->setDevice(QMediaDevices::audioOutputs()[index]);
			return true;
		}

		[[nodiscard]]
		auto setOutputDevice(const AudioOutputDevice& device) -> bool final {
			return setOutputDevice(device.index());
		}

		auto setStreamUuid(QString /*uuid*/) -> void final {}

		auto setMuted(bool mute) -> void final {
			m_output->setMuted(mute);
		}

		auto setCategory(Category /*category*/) -> void final {}

		auto connectToMediaPlayer(QMediaPlayer* player) -> void final {
			player->setAudioOutput(m_output);
			SinkNode::connectToMediaPlayer(player);
		}

		auto disconnectFromMediaPlayer(QMediaPlayer* player) -> void final {
			player->setAudioOutput(nullptr);
			SinkNode::disconnectFromMediaPlayer(player);
		}

	  signals:
		auto volumeChanged(qreal volume) -> void;
		auto audioDeviceFailed() -> void;
		void mutedChanged(bool _t1) override;

	  private:
		QAudioOutput* m_output;
	};
} // namespace Phonon::Native

#include "audiooutput.moc"
