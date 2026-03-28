#ifndef _REAPER_PLUGIN_FUNCTIONS_H_
#define _REAPER_PLUGIN_FUNCTIONS_H_

#include "reaper_plugin.h"

// Forward declarations for REAPER types
class MediaTrack;
class ReaProject;
class MediaItem;
class MediaItem_Take;
class TrackEnvelope;
class PCM_source;

// Function pointers
extern const char* (*GetAppVersion)();
extern void (*ShowConsoleMsg)(const char* msg);
extern int (*ShowMessageBox)(const char* msg, const char* title, int type);
extern int (*CountTracks)(ReaProject* proj);
extern MediaTrack* (*GetTrack)(ReaProject* proj, int trackidx);
extern bool (*GetSetMediaTrackInfo_String)(MediaTrack* tr, const char* parmname, char* string, bool setnewvalue);
extern double (*GetMediaTrackInfo_Value)(MediaTrack* tr, const char* parmname);
extern int (*GetTrackColor)(MediaTrack* track);
extern MediaTrack* (*GetMasterTrack)(ReaProject* proj);
extern bool (*GetSetProjectInfo_String)(ReaProject* project, const char* desc, char* val, bool is_set);
extern double (*GetSetProjectInfo)(ReaProject* project, const char* desc, double val, bool is_set);
extern void (*GetProjectName)(ReaProject* proj, char* bufOut, int bufOut_sz);
extern void (*GetProjectPath)(char* bufOut, int bufOut_sz);
extern const char* (*EnumProjects)(int idx, char* projfn, int projfn_sz);
extern double (*GetProjectLength)(ReaProject* proj);
extern int (*GetTrackNumMediaItems)(MediaTrack* tr);
extern MediaItem* (*GetTrackMediaItem)(MediaTrack* tr, int itemidx);
extern MediaItem_Take* (*GetActiveTake)(MediaItem* item);
extern bool (*GetSetMediaItemTakeInfo_String)(MediaItem_Take* tk, const char* parmname, char* string, bool setnewvalue);
extern double (*GetMediaItemTakeInfo_Value)(MediaItem_Take* tk, const char* parmname);
extern MediaItem* (*GetMediaItemTake_Item)(MediaItem_Take* tk);
extern double (*GetMediaItemInfo_Value)(MediaItem* item, const char* parmname);
extern PCM_source* (*GetMediaItemTake_Source)(MediaItem_Take* tk);
extern void (*GetMediaSourceFileName)(PCM_source* source, char* filenamebuf, int filenamebuf_sz);
extern int (*EnumProjectMarkers3)(ReaProject* proj, int idx, bool* isrgn, double* pos, double* rgnend, const char** name, int* markindex, int* color);
extern int (*GetTrackNumSends)(MediaTrack* tr, int category);
extern double (*GetTrackSendInfo_Value)(MediaTrack* tr, int category, int sendidx, const char* parmname);
extern TrackEnvelope* (*GetTrackEnvelopeByName)(MediaTrack* track, const char* envname);
extern int (*CountTrackEnvelopes)(MediaTrack* track);
extern TrackEnvelope* (*GetTrackEnvelope)(MediaTrack* track, int envidx);
extern TrackEnvelope* (*GetTakeEnvelopeByName)(MediaItem_Take* take, const char* envname);
extern int (*CountTakeEnvelopes)(MediaItem_Take* take);
extern TrackEnvelope* (*GetTakeEnvelope)(MediaItem_Take* take, int envidx);
extern bool (*GetEnvelopeName)(TrackEnvelope* env, char* buf, int buflen);
extern bool (*GetEnvelopeStateChunk)(TrackEnvelope* env, char* str, int str_sz, bool isundo);
extern int (*CountEnvelopePoints)(TrackEnvelope* envelope);
extern bool (*GetEnvelopePoint)(TrackEnvelope* envelope, int ptidx, double* timeOut, double* valueOut, int* shapeOut, double* tensionOut, bool* selectedOut);
extern bool (*GetSetEnvelopeInfo_String)(TrackEnvelope* env, const char* parmname, char* stringNeedBig, bool setNewValue);
extern int (*CountAutomationItems)(TrackEnvelope* env);
extern int (*CountEnvelopePointsEx)(TrackEnvelope* envelope, int autoitem_idx);
extern bool (*GetEnvelopePointEx)(TrackEnvelope* envelope, int autoitem_idx, int ptidx, double* timeOut, double* valueOut, int* shapeOut, double* tensionOut, bool* selectedOut);
extern double (*ScaleFromEnvelopeMode)(int mode, double val);
extern int (*plugin_register)(const char* name, void* inf);
extern int (*GetMasterMuteSoloFlags)();
extern int (*TrackFX_GetCount)(MediaTrack* tr);
extern bool (*TrackFX_GetFXName)(MediaTrack* tr, int fx, char* bufOut, int bufOut_sz);
extern bool (*TrackFX_GetParamName)(MediaTrack* tr, int fx, int param, char* bufOut, int bufOut_sz);
extern double (*TrackFX_GetParam)(MediaTrack* tr, int fx, int param, double* minvalOut, double* maxvalOut);
extern bool (*TrackFX_GetEnabled)(MediaTrack* tr, int fx);
extern bool (*TrackFX_GetNamedConfigParm)(MediaTrack* tr, int fx, const char* parmname, char* bufOut, int bufOut_sz);
extern int (*TrackFX_GetNumParams)(MediaTrack* tr, int fx);
extern TrackEnvelope* (*GetFXEnvelope)(MediaTrack* tr, int fx, int param, bool create);
extern int (*TakeFX_GetCount)(MediaItem_Take* tk);
extern bool (*TakeFX_GetFXName)(MediaItem_Take* tk, int fx, char* bufOut, int bufOut_sz);
extern bool (*TakeFX_GetParamName)(MediaItem_Take* tk, int fx, int param, char* bufOut, int bufOut_sz);
extern double (*TakeFX_GetParam)(MediaItem_Take* tk, int fx, int param, double* minvalOut, double* maxvalOut);
extern bool (*TakeFX_GetEnabled)(MediaItem_Take* tk, int fx);
extern bool (*TakeFX_GetNamedConfigParm)(MediaItem_Take* tk, int fx, const char* parmname, char* bufOut, int bufOut_sz);
extern int (*TakeFX_GetNumParams)(MediaItem_Take* tk, int fx);
extern TrackEnvelope* (*TakeFX_GetEnvelope)(MediaItem_Take* tk, int fx, int param, bool create);
extern bool (*TrackFX_GetPreset)(MediaTrack* tr, int fx, char* presetnameOut, int presetnameOut_sz);
extern double (*TrackFX_GetParamEx)(MediaTrack* track, int fx, int param, double* minvalOut, double* maxvalOut, double* midvalOut);
extern bool (*TakeFX_GetPreset)(MediaItem_Take* tk, int fx, char* presetnameOut, int presetnameOut_sz);
extern double (*TakeFX_GetParamEx)(MediaItem_Take* take, int fx, int param, double* minvalOut, double* maxvalOut, double* midvalOut);
extern bool (*GetMediaSourceType)(PCM_source* source, char* typebuf, int typebuf_sz);
extern bool (*GetItemStateChunk)(MediaItem* item, char* str, int str_sz, bool isundo);
extern bool (*GetTrackStateChunk)(MediaTrack* track, char* str, int str_sz, bool isundo);
extern void (*ColorFromNative)(int col, int* r, int* g, int* b);
extern int (*ColorToNative)(int r, int g, int b);
extern double (*TimeMap_curFrameRate)(ReaProject* proj, bool* dropframeOut);
extern int (*GetNumTakeMarkers)(MediaItem_Take* take);
extern double (*GetTakeMarker)(MediaItem_Take* take, int idx, char* namebuf, int namebuf_sz, int* colorOut);

// Helper macro to import functions
#define IMPAPI(x) if (!((*((void **)&(x)) = (void *)rec->GetFunc(#x)))) errcnt++;

#endif // _REAPER_PLUGIN_FUNCTIONS_H_
