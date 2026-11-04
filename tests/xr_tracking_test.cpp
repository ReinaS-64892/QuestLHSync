// Hardware-free integration tests for the OpenXR -> solver input boundary.
// Compile the real app with a renamed entry point, then supply mock runtime APIs.
#define main questlhsync_app_main
#include "../src/xr/xr_main.cpp"
#undef main

#include <cassert>
#include <limits>

// Exercise the production candidate selection with two identical HMD models.
struct HeadsetLinkTest {
  static void Run() {
    HeadsetLink link(nullptr, [](const std::string &) {}, [](double, const std::string &) {});
    link.SetFamily("Steam Frame");
    link.SetPreferred("other-frame");
    link.SetStreamer("WiVRn");
    link.known_ = {{"192.0.2.2", "", "Steam Frame", "WiVRn"}};
    std::vector<HeadsetLink::Found> replies{
        {"192.0.2.2", "other-frame", "Steam Frame", 47280},
        {"192.0.2.3", "third-frame", "Steam Frame", 47280}};
    link.SetHosts({"127.0.0.1"});
    link.quiet_ips_.insert("127.0.0.1");
    auto candidates = link.Candidates(replies);
    assert(candidates.size() == 1 && candidates[0].ip == "127.0.0.1");
    assert(candidates[0].port == 47280);
    assert(!link.Others("127.0.0.1"));
    std::vector<HeadsetLink::Found> found;
    assert(!link.Discover(found) && found.empty()); // No broadcast with explicit hosts.
    link.wrong_ips_.insert("127.0.0.1");
    assert(link.Candidates(replies).empty()); // Rejection cannot fall back to another Frame.
    link.wrong_ips_.clear();
    link.SetHosts({"frame.local", "127.0.0.1"});
    candidates = link.Candidates(replies);
    assert(candidates.size() == 2);
    assert(candidates[0].ip == "frame.local" && candidates[1].ip == "127.0.0.1");
    assert(link.Others("127.0.0.1"));
    link.SetHosts({});
    candidates = link.Candidates(replies);
    assert(candidates.size() == 2); // Auto discovery is restored when hosts are cleared.
    assert(candidates[0].ip == "192.0.2.2");
    assert(link.Others("192.0.2.2"));
  }
};

namespace {
Rig stage, offset, head;
std::map<XrSpace, Rig> raw;
std::set<XrSpace> invalid;
int stage_reads, origin_reads;
bool change_stage, change_origin, fail_stage, fail_origin;
std::vector<uint32_t> writes;
Rig written;

Rig Pose(double yaw, V3 p) { return {ToQuat(Ry(yaw) * Tilt(0.12, -0.07)), p}; }
void Near(const Rig &a, const Rig &b) {
  assert(norm(a.p - b.p) < 1e-5);
  assert(QuatDeg(a.q, b.q) < 1e-3);
}

mnd_result_t GetStage(mnd_root_t *, mnd_reference_space_type_t type, mnd_pose_t *out) {
  assert(type == MND_SPACE_REFERENCE_TYPE_STAGE);
  if (fail_stage) return MND_ERROR_OPERATION_FAILED;
  Rig value = stage;
  if (++stage_reads > 1 && change_stage) value.p.x += 1;
  *out = ToMnd(value);
  return MND_SUCCESS;
}
mnd_result_t GetOrigin(mnd_root_t *, uint32_t origin, mnd_pose_t *out) {
  assert(origin == 42);
  if (fail_origin) return MND_ERROR_OPERATION_FAILED;
  Rig value = offset;
  if (++origin_reads > 1 && change_origin) value.p.z += 1;
  *out = ToMnd(value);
  return MND_SUCCESS;
}
mnd_result_t SetOrigin(mnd_root_t *, uint32_t origin, const mnd_pose_t *value) {
  writes.push_back(origin);
  written = FromMnd(*value);
  return MND_SUCCESS;
}
XrResult XRAPI_CALL LocateMock(XrSpace space, XrSpace, XrTime time, XrSpaceLocation *out) {
  assert(time == 123456789);
  const bool hmd = space == reinterpret_cast<XrSpace>(1);
  Rig pose = Inv(stage) * (hmd ? head : offset * raw.at(space));
  out->pose = {{(float)pose.q.x, (float)pose.q.y, (float)pose.q.z, (float)pose.q.w},
               {(float)pose.p.x, (float)pose.p.y, (float)pose.p.z}};
  out->locationFlags = invalid.count(space) ? 0 :
      XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
      XR_SPACE_LOCATION_POSITION_TRACKED_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
  return XR_SUCCESS;
}
Dev Device(int id, const char *serial, uint32_t origin, bool station = false, bool hmd = false) {
  Dev d{};
  d.id = id;
  d.space = reinterpret_cast<XrSpace>((uintptr_t)id);
  d.serial = serial;
  d.origin = origin;
  d.station = station;
  d.hmd = hmd;
  d.lighthouse = !hmd;
  return d;
}

void TrackingTests() {
  Xr xr;
  xr.xrLocateSpace = LocateMock;
  Mnd mnd;
  mnd.get_ref = GetStage;
  mnd.get_origin = GetOrigin;
  mnd.set_origin = SetOrigin;
  std::vector<Dev> devs = {Device(1, "head", 7, false, true), Device(2, "LHB-12345678", 42, true),
                           Device(3, "LHB-ABCDEF00", 42, true), Device(4, "LHR-body", 42)};
  raw[devs[1].space] = Pose(0.4, {2, 3, -1});
  raw[devs[2].space] = Pose(-0.8, {-2, 2.5, 4});
  raw[devs[3].space] = Pose(0.2, {0.2, 0.7, -0.3});
  head = Pose(-0.3, {1, 1.7, 2});
  auto read = [&] {
    stage_reads = origin_reads = 0;
    return ReadTrackingFrame(xr, mnd, devs, 123456789);
  };
  for (int i = 0; i < 3; i++) {
    // User recenter and our own driver-space correction must not enter raw BS
    // positions, including when the applied correction changes between batches.
    stage = Pose(0.2 + i, {double(i), 0.3, -2});
    offset = Pose(-0.6 - i, {-3, double(i), 1});
    auto f = read();
    assert(f.issue.empty() && f.have_hmd && f.origin == 42);
    assert(f.stations.size() == 2 && f.bodies.size() == 1);
    Near(f.hmd, head);
    for (int k : {1, 2}) {
      const auto &station = f.stations.at(devs[k].serial);
      Near({ToQuat(station.second), station.first}, raw.at(devs[k].space));
    }
    assert(norm(f.bodies.at(4) - raw.at(devs[3].space).p) < 1e-5);
    Transform correction{};
    correction.active = true;
    correction.q = ToQuat(Ry(0.8));
    correction.t = {0.2, -0.1, 0.4};
    writes.clear();
    assert(ApplyCorrection(mnd, f, correction));
    assert(writes == std::vector<uint32_t>{42});
    Near(written, {correction.q, correction.t});
    // A freshly connected runtime must be written even if the desired
    // transform equals the value applied to the previous runtime.
    f.offset = written;
    writes.clear();
    assert(ApplyCorrection(mnd, f, correction) && writes.empty());
    f.offset = {};
    assert(ApplyCorrection(mnd, f, correction) && writes.size() == 1);
  }
  invalid.insert(devs[1].space);
  assert(read().stations.size() == 1);
  invalid.insert(devs[2].space);
  auto empty = read();
  assert(empty.stations.empty() && empty.have_hmd);
  Transform correction{};
  correction.active = true;
  correction.t = {1, 2, 3};
  writes.clear();
  assert(ApplyCorrection(mnd, empty, correction) && writes.empty());
  invalid.clear();
  assert(read().stations.size() == 2); // recovered measurements
  change_stage = true;
  auto changed = read();
  assert(!changed.have_hmd && changed.stations.empty() && changed.origin == UINT32_MAX);
  change_stage = false;
  change_origin = true;
  changed = read();
  assert(!changed.have_hmd && changed.stations.empty());
  change_origin = false;
  fail_stage = true;
  assert(read().origin == UINT32_MAX);
  fail_stage = false;
  fail_origin = true;
  assert(read().stations.empty());
  fail_origin = false;
  devs[0].origin = 42;
  assert(read().origin == UINT32_MAX); // never move the HMD
  devs[0].origin = 7;
  devs[2].origin = 43;
  assert(read().origin == UINT32_MAX); // unrelated driver frames cannot be merged
  devs[2].origin = UINT32_MAX;
  assert(read().origin == UINT32_MAX);
  devs[2].origin = 42;
  raw[devs[1].space].p.x = std::numeric_limits<double>::quiet_NaN();
  assert(read().stations.size() == 1);
  assert(!ReadTrackingFrame(xr, mnd, devs, 0).have_hmd);
}

// Exercise the real enumerator against immutable runtime snapshots. In
// particular, the same generation number must not hide a late base station.
std::vector<Dev> devices;
struct Root { std::vector<Dev> devices; };
int creates, destroys;
mnd_result_t Create(mnd_root_t **root) {
  *root = reinterpret_cast<mnd_root_t *>(new Root{devices});
  ++creates;
  return MND_SUCCESS;
}
void Destroy(mnd_root_t **root) {
  delete reinterpret_cast<Root *>(*root);
  *root = nullptr;
  ++destroys;
}
mnd_result_t Count(mnd_root_t *root, uint32_t *out) {
  *out = reinterpret_cast<Root *>(root)->devices.size();
  return MND_SUCCESS;
}
mnd_result_t OriginCount(mnd_root_t *, uint32_t *out) { *out = 2; return MND_SUCCESS; }
mnd_result_t OriginName(mnd_root_t *, uint32_t, const char **out) { *out = "origin"; return MND_SUCCESS; }
mnd_result_t Role(mnd_root_t *, const char *, int32_t *out) { *out = 0; return MND_SUCCESS; }
mnd_result_t Info(mnd_root_t *root, uint32_t index, uint32_t *id, const char **name) {
  *id = index;
  *name = reinterpret_cast<Root *>(root)->devices.at(index).name.c_str();
  return MND_SUCCESS;
}
mnd_result_t Serial(mnd_root_t *root, uint32_t index, mnd_property_t, const char **out) {
  *out = reinterpret_cast<Root *>(root)->devices.at(index).serial.c_str();
  return MND_SUCCESS;
}
mnd_result_t Origin(mnd_root_t *root, uint32_t index, mnd_property_t, uint32_t *out) {
  *out = reinterpret_cast<Root *>(root)->devices.at(index).origin;
  return MND_SUCCESS;
}
XrResult XRAPI_CALL CreateList(XrSession, const XrCreateXDevListInfoMNDX *, XrXDevListMNDX *out) {
  *out = reinterpret_cast<XrXDevListMNDX>(1);
  return XR_SUCCESS;
}
XrResult XRAPI_CALL DestroyList(XrXDevListMNDX) { return XR_SUCCESS; }
XrResult XRAPI_CALL EnumerateDevices(XrXDevListMNDX, uint32_t capacity, uint32_t *count, XrXDevIdMNDX *ids) {
  *count = devices.size();
  for (uint32_t i = 0; i < capacity; i++) ids[i] = i;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Properties(XrXDevListMNDX, const XrGetXDevInfoMNDX *info, XrXDevPropertiesMNDX *out) {
  const auto &d = devices.at(info->id);
  snprintf(out->name, sizeof(out->name), "%s", d.name.c_str());
  snprintf(out->serial, sizeof(out->serial), "%s", d.serial.c_str());
  out->canCreateSpace = true;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL CreateSpace(XrSession, const XrCreateXDevSpaceInfoMNDX *info, XrSpace *out) {
  *out = reinterpret_cast<XrSpace>((uintptr_t)(info->id + 1));
  return XR_SUCCESS;
}
XrResult XRAPI_CALL DestroySpace(XrSpace) { return XR_SUCCESS; }

void EnumerationTests() {
  Xr xr;
  xr.xrCreateXDevListMNDX = CreateList;
  xr.xrDestroyXDevListMNDX = DestroyList;
  xr.xrEnumerateXDevsMNDX = EnumerateDevices;
  xr.xrGetXDevPropertiesMNDX = Properties;
  xr.xrCreateXDevSpaceMNDX = CreateSpace;
  xr.xrDestroySpace = DestroySpace;
  Mnd mnd;
  mnd.create = Create; mnd.destroy = Destroy; mnd.count = Count;
  mnd.origin_count = OriginCount; mnd.origin_name = OriginName;
  mnd.role = Role; mnd.info = Info; mnd.info_str = Serial; mnd.info_u32 = Origin;
  devices = {Device(1, "head", 0, false, true)};
  std::vector<Dev> result;
  std::map<uint32_t, std::string> origins;
  assert(Enumerate(xr, mnd, result, origins) && result.size() == 1 && result[0].hmd);
  devices.push_back(Device(2, "LHB-12345678", 1, true));
  assert(Enumerate(xr, mnd, result, origins) && result.size() == 2);
  assert(result[1].station && result[1].lighthouse && result[1].origin == 1);
  devices.resize(1);
  assert(Enumerate(xr, mnd, result, origins) && result.size() == 1);
  mnd.Disconnect();
  assert(creates == 3 && destroys == creates);
}
} // namespace

int main() {
  HeadsetLinkTest::Run();
  TrackingTests();
  EnumerationTests();
  puts("OpenXR tracking tests passed");
}
