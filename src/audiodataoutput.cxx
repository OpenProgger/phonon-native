module;

#include <QAudioDecoder>
#include <QMediaPlayer>
#include <QtCore/qtmochelpers.h>
#include <phonon/AudioDataOutput>
#include <phonon/audiodataoutputinterface.h>

export module phonon_native:audiodataoutput;

import :sinknode;

export namespace Phonon::Native {
	class AudioDataOutput final:
		public QObject,
		public AudioDataOutputInterface,
		public SinkNode {
		Q_OBJECT
		Q_INTERFACES(Phonon::AudioDataOutputInterface)

	  public:
		explicit AudioDataOutput(QObject* parent):
			QObject{parent}, m_decoder{new QAudioDecoder{parent}} {
			QAudioFormat format{};
			format.setChannelConfig(
				QAudioFormat::defaultChannelConfigForChannelCount(2));
			format.setChannelCount(2);
			format.setSampleFormat(QAudioFormat::Int16);
			format.setSampleRate(44'100);
			m_decoder->setAudioFormat(format);
			connect(
				m_decoder,
				&QAudioDecoder::bufferReady,
				this,
				[=, this]() { m_buffer << m_decoder->read(); },
				Qt::AutoConnection);
		}

		~AudioDataOutput() final = default;
		AudioDataOutput(const AudioDataOutput&) = delete;
		AudioDataOutput(AudioDataOutput&&) = delete;
		auto operator=(const AudioDataOutput&) -> AudioDataOutput& = delete;
		auto operator=(AudioDataOutput&&) -> AudioDataOutput& = delete;

		Phonon::AudioDataOutput* frontendObject() const final {
			return m_frontend;
		}

		void setFrontendObject(Phonon::AudioDataOutput* frontend) final {
			m_frontend = frontend;
		}

		auto connectToMediaPlayer(QMediaPlayer* player) -> void final {
			connect(
				player,
				&QMediaPlayer::sourceChanged,
				this,
				[=, this](const QUrl& source) {
					m_buffer.clear();
					m_decoder->setSource(source);
					m_decoder->start();
				},
				Qt::AutoConnection);
			connect(
				player,
				&QMediaPlayer::hasVideoChanged,
				this,
				[=, this](bool enabled) {
					if(enabled) {
						m_decoder->stop();
						m_buffer.clear();
					}
				},
				Qt::AutoConnection);
			connect(player,
				&QMediaPlayer::positionChanged,
				this,
				&AudioDataOutput::onPositionChange,
				Qt::AutoConnection);
			SinkNode::connectToMediaPlayer(player);
		}

		auto disconnectFromMediaPlayer(QMediaPlayer* player) -> void final {
			disconnect(player, nullptr, this, nullptr);
			SinkNode::disconnectFromMediaPlayer(player);
		}

	  signals:
		auto endOfMedia(int remainingSamples) -> void;
		auto dataReady(
			const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16>>& data)
			-> void;

	  private:
		Phonon::AudioDataOutput* m_frontend;
		QAudioDecoder* m_decoder;
		QVector<QAudioBuffer> m_buffer;
		long m_dataSize{512};

	  public slots:

		auto setDataSize(int size) -> void {
			m_dataSize = size;
		}

	  private slots:

		auto onPositionChange(qint64 position) -> void {
			for(auto i{1}; i < m_buffer.size(); i++) {
				if((m_buffer[i].startTime() / 1000) > position) {
					auto audioData{m_buffer[i - 1].constData<qint16>()};
					QVector<qint16> leftChannel;
					QVector<qint16> rightChannel;
					for(auto frame{0};
						frame < m_dataSize
						&& frame < (m_buffer[i - 1].sampleCount() / 2);
						frame++) {
						leftChannel << audioData[2 * frame];
						rightChannel << audioData[2 * frame + 1];
					}

					QMap<Phonon::AudioDataOutput::Channel, QVector<qint16>>
						data;
					data.insert(
						Phonon::AudioDataOutput::LeftChannel, leftChannel);
					data.insert(
						Phonon::AudioDataOutput::RightChannel, rightChannel);
					emit dataReady(data);
					break;
				}
			}
		}
	};
} // namespace Phonon::Native

#include "audiodataoutput.moc"
