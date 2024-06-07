module;

#include <QAudioDevice>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QMimeType>
#include <QPluginMetaDataV2>
#include <QtCore/qtmochelpers.h>
#include <phonon/BackendInterface>
#include <phonon/GlobalDescriptionContainer>

export module phonon_native;
import :audiooutput;
import :audiodataoutput;
import :mediaobject;
import :sinknode;
import :videowidget;
import :volumefadereffect;

using Qt::Literals::StringLiterals::operator""_L1;
using Qt::Literals::StringLiterals::operator""_ba;

namespace Phonon::Native {

	class Backend final: public QObject, public BackendInterface {
		Q_OBJECT
		Q_PLUGIN_METADATA(IID "org.kde.phonon.native" FILE "phonon-native.json")
		Q_INTERFACES(Phonon::BackendInterface)

	  public:
		Backend(): Backend(nullptr, {}) {}

		Backend(const Backend&) = delete;
		Backend(Backend&&) = delete;
		auto operator=(const Backend&) -> Backend& = delete;
		auto operator=(Backend&&) -> Backend& = delete;

		explicit Backend(QObject* parent, const QVariantList& /*args*/):
			QObject{parent} {
			setProperty("identifier", "phonon_native"_L1);
			setProperty("backendName", "Native"_L1);
			setProperty("backendComment", "qtmultimedia backend for Phonon"_L1);
			setProperty("backendVersion", QLatin1String{PHONON_MPV_VERSION});
			setProperty("backendIcon", "qtcreator"_L1);
			setProperty("backendWebsite",
				"https://github.com/OpenProgger/phonon-native"_L1);

			qDebug() << "Initializing Phonon-Native " << PHONON_MPV_VERSION;

			for(const auto& device: QMediaDevices::audioOutputs()) {
				m_devices.append({AudioOutputDeviceType,
					{device.id(), device.description()}});
			}
			for(const auto& device: QMediaDevices::audioInputs()) {
				m_devices.append({AudioCaptureDeviceType,
					{device.id(), device.description()}});
			}
			for(const auto& device: QMediaDevices::videoInputs()) {
				m_devices.append({VideoCaptureDeviceType,
					{device.id(), device.description()}});
			}
		}

		~Backend() final {
			if(GlobalAudioChannels::self) {
				delete GlobalAudioChannels::self;
			}
			if(GlobalSubtitles::self) {
				delete GlobalSubtitles::self;
			}
		}

		auto createObject(BackendInterface::Class classType, QObject* parent,
			const QList<QVariant>& /*args*/) -> QObject* final {
			switch(classType) {
				case MediaObjectClass:
					return new MediaObject{parent};
				case AudioOutputClass:
					return new AudioOutput{parent};
				case AudioDataOutputClass:
					return new AudioDataOutput(parent);
				case EffectClass:
					/* We have no effects */
					return nullptr;
				case VideoWidgetClass:
					return new VideoWidget(qobject_cast<QWidget*>(parent));
				case VolumeFaderEffectClass:
					return new VolumeFaderEffect(parent);
				case VisualizationClass:
				case VideoDataOutputClass:
				case VideoGraphicsObjectClass:
					break;
			}

			qDebug() << "Backend class" << classType
					 << "is not supported by Phonon Native :(";
			return nullptr;
		}

		[[nodiscard]]
		auto availableMimeTypes() const -> QStringList final {
			QStringList list;

			for(const auto& format:
				QMediaFormat{QMediaFormat::UnspecifiedFormat}
					.supportedFileFormats(QMediaFormat::Decode)) {
				list << QMediaFormat{format}.mimeType().name();
			}

			return list;
		}

		[[nodiscard]]
		auto objectDescriptionIndexes(
			ObjectDescriptionType type) const -> QList<int> final {
			QList<int> list;

			switch(type) {
				case AudioChannelType:
					list << GlobalAudioChannels::instance()->globalIndexes();
					break;
				case AudioOutputDeviceType:
				case AudioCaptureDeviceType:
				case VideoCaptureDeviceType:
					for(auto i{0}; i < m_devices.size(); i++) {
						if(m_devices[i].first == type) {
							list << i;
						}
					}
					break;
				case EffectType:
					/* We have no effects */
					break;
				case SubtitleType:
					list << GlobalSubtitles::instance()->globalIndexes();
					break;
			}

			return list;
		}

		[[nodiscard]]
		auto objectDescriptionProperties(ObjectDescriptionType type,
			int index) const -> QHash<QByteArray, QVariant> final {
			QHash<QByteArray, QVariant> properties;

			switch(type) {
				case AudioChannelType:
					{
						const AudioChannelDescription description =
							GlobalAudioChannels::instance()->fromIndex(index);
						properties.insert("name"_ba, description.name());
						properties.insert(
							"description"_ba, description.description());
					}
					break;
				case AudioOutputDeviceType:
				case AudioCaptureDeviceType:
				case VideoCaptureDeviceType:
					properties.insert("name"_ba, m_devices[index].second.first);
					properties.insert(
						"description"_ba, m_devices[index].second.second);
					properties.insert("isAdvanced"_ba, true);
					properties.insert("deviceAccessList"_ba,
						QVariant::fromValue<DeviceAccessList>(
							DeviceAccessList{m_devices[index].second}));
					properties.insert("discovererIcon"_ba, "qtcreator");
					if(m_devices[index].first == AudioOutputDeviceType) {
						properties.insert("icon"_ba, "audio-card"_L1);
					} else if(m_devices[index].first
							  == AudioCaptureDeviceType) {
						properties.insert("hasaudio"_ba, true);
						properties.insert(
							"icon"_ba, "audio-input-microphone"_L1);
					} else {
						properties.insert("hasvideo"_ba, true);
						properties.insert("icon"_ba, "camera-web"_L1);
					}
					break;
				case EffectType:
					{ /* We have no effects */
					}
					break;
				case SubtitleType:
					{
						const SubtitleDescription description{
							GlobalSubtitles::instance()->fromIndex(index)};
						properties.insert("name"_ba, description.name());
						properties.insert(
							"description"_ba, description.description());
						properties.insert(
							"type"_ba, description.property("type"));
					}
					break;
			}

			return properties;
		}

		auto startConnectionChange(QSet<QObject*> /*unused*/) -> bool final {
			return true;
		}

		auto connectNodes(QObject* source, QObject* sink) -> bool final {
			SinkNode* sinkNode{dynamic_cast<SinkNode*>(sink)};
			if(sinkNode) {
				MediaObject* mediaObject{qobject_cast<MediaObject*>(source)};
				if(mediaObject) {
					sinkNode->connectToMediaPlayer(mediaObject->m_player);
					return true;
				}

				SinkNode* sinkSourceNode{dynamic_cast<SinkNode*>(source)};
				if(sinkSourceNode) {
					sinkNode->connectToMediaPlayer(
						sinkSourceNode->mediaPlayer());
					return true;
				}
			}

			return false;
		}

		auto disconnectNodes(QObject* source, QObject* sink) -> bool final {
			SinkNode* sinkNode{dynamic_cast<SinkNode*>(sink)};
			if(sinkNode) {
				MediaObject* const mediaObject{
					qobject_cast<MediaObject*>(source)};
				if(mediaObject) {
					sinkNode->disconnectFromMediaPlayer(mediaObject->m_player);
					return true;
				}

				SinkNode* sinkSourceNode{dynamic_cast<SinkNode*>(source)};
				if(sinkSourceNode) {
					sinkNode->disconnectFromMediaPlayer(
						sinkSourceNode->mediaPlayer());
					return true;
				}
			}

			return false;
		}

		auto endConnectionChange(QSet<QObject*> /*unused*/) -> bool final {
			return true;
		}

	  signals:
		auto objectDescriptionChanged(ObjectDescriptionType /*unused*/) -> void;

	  private:
		QVector<QPair<ObjectDescriptionType, DeviceAccess>> m_devices;
	};

} // namespace Phonon::Native

#include "backend.moc"
