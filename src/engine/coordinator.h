// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Platform-independent coordinator class definition. This is subclassed by the
// platform-specific coordinator classes.

#ifndef FIRMAMENT_ENGINE_COORDINATOR_H
#define FIRMAMENT_ENGINE_COORDINATOR_H

#include <string>
#include <map>
#include <utility>
#include <vector>

// XXX(malte): Think about the Boost dependency!
#ifdef __PLATFORM_HAS_BOOST__
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/uuid/uuid.hpp>
#else
// Currently this won't build if __PLATFORM_HAS_BOOST__ is not defined.
#error __PLATFORM_HAS_BOOST__ not set, so cannot build coordinator!
#endif

#include "base/common.h"
#include "base/types.h"
#include "base/job_desc.pb.h"
#include "base/task_desc.pb.h"
#include "base/reference_desc.pb.h"
#include "base/resource_desc.pb.h"
#include "engine/node.h"
// XXX(malte): include order dependency
#include "platforms/unix/common.h"
#include "messages/heartbeat_message.pb.h"
#include "messages/registration_message.pb.h"
#include "messages/task_state_message.pb.h"
#include "misc/messaging_interface.h"
#include "platforms/common.h"
#include "platforms/unix/signal_handler.h"
#include "platforms/unix/stream_sockets_adapter.h"
#include "platforms/unix/stream_sockets_adapter-inl.h"
#ifdef __HTTP_UI__
#include "engine/coordinator_http_ui.h"
#endif
#include "engine/scheduler_interface.h"
#include "engine/simple_scheduler.h"
#include "engine/object_store_interface.h"
#include "engine/topology_manager.h"
#ifdef __SIMULATE_SYNTHETIC_DTG__
#include "sim/simple_dtg_generator.h"
#endif

namespace firmament {

//using __gnu_cxx::hash_map;

using machine::topology::TopologyManager;
using platform_unix::SignalHandler;
using platform_unix::streamsockets::StreamSocketsChannel;
using platform_unix::streamsockets::StreamSocketsAdapter;
using scheduler::SchedulerInterface;
using scheduler::SimpleScheduler;
using store::ObjectStoreInterface;

#ifdef __HTTP_UI__
// Forward declaration
namespace webui {
class CoordinatorHTTPUI;
}  // namespace webui
using webui::CoordinatorHTTPUI;
#endif

class Coordinator : public Node,
                    public boost::enable_shared_from_this<Coordinator> {
 public:
  explicit Coordinator(PlatformID platform_id);
  virtual ~Coordinator();
  void Run();
  const JobDescriptor* DescriptorForJob(const string& job_id);
  void Shutdown(const string& reason);
  const string SubmitJob(const JobDescriptor& job_descriptor);

  // Gets a pointer to the resource descriptor for an associated resource
  // (including the coordinator itself); returns NULL if the resource is not
  // associated or the coordinator itself.
  ResourceDescriptor* GetResource(ResourceID_t res_id) {
    VLOG(1) << "res_id: " << res_id << ", " << uuid_;
    if (res_id == uuid_)
      return &resource_desc_;
    pair<ResourceDescriptor, uint64_t>* res =
        FindOrNull(*associated_resources_, res_id);
    return (res ? &(res->first) : NULL);
  }
  vector<ResourceDescriptor> associated_resources() {
    vector<ResourceDescriptor> ref_vec;
    for (ResourceMap_t::const_iterator res_iter =
         associated_resources_->begin();
         res_iter != associated_resources_->end();
         ++res_iter) {
      ref_vec.push_back(res_iter->second.first);
    }
    return ref_vec;
  };
  vector<JobDescriptor> active_jobs() {
    vector<JobDescriptor> jd_vec;
    for (JobMap_t::const_iterator job_iter =
         job_table_->begin();
         job_iter != job_table_->end();
         ++job_iter) {
      jd_vec.push_back(job_iter->second);
    }
    return jd_vec;
  }
  vector<shared_ptr<TaskDescriptor> > active_tasks() {
    vector<shared_ptr<TaskDescriptor> > td_vec;
    for (TaskMap_t::const_iterator task_iter =
         task_table_->begin();
         task_iter != task_table_->end();
         ++task_iter) {
      td_vec.push_back(task_iter->second);
    }
    return td_vec;
  }

 protected:
  void AddJobsTasksToTaskTable(RepeatedPtrField<TaskDescriptor>* tasks);
  bool RegisterWithCoordinator(
      shared_ptr<StreamSocketsChannel<BaseMessage> > chan);
  void DetectLocalResources();
  void HandleIncomingMessage(BaseMessage *bm);
  void HandleHeartbeat(const HeartbeatMessage& msg);
  void HandleRegistrationRequest(const RegistrationMessage& msg);
  void HandleTaskStateChange(const TaskStateMessage& msg);
#ifdef __HTTP_UI__
  void InitHTTPUI();
#endif

#ifdef __HTTP_UI__
  scoped_ptr<CoordinatorHTTPUI> c_http_ui_;
#endif
  scoped_ptr<TopologyManager> topology_manager_;
  // A map of resources associated with this coordinator.
  // The key is a resource UUID, the value a pair.
  // The first component of the pair is the resource descriptor, the second is
  // the timestamp when the latest heartbeat or message was received from this
  // resource..
  shared_ptr<ResourceMap_t> associated_resources_;
  // A map of all jobs known to this coordinator, indexed by their job ID.
  // Key is the job ID, value a ResourceDescriptor.
  // Currently, this table grows ad infinitum.
  shared_ptr<JobMap_t> job_table_;
  // A map of all data objects known to the coordinator.
  // TODO(malte): This may move to the store layer.
  shared_ptr<DataObjectMap_t> object_table_;
  // A map of all tasks that the coordinator currently knows about.
  // TODO(malte): Think about GC'ing this.
  shared_ptr<TaskMap_t> task_table_;
  // This coordinator's own resource descriptor.
  ResourceDescriptor resource_desc_;
  ResourceID_t uuid_;
  // The local scheduler object. A coordinator may not have a scheduler, in
  // which case this will be a stub that defers to another scheduler.
  // TODO(malte): Work out the detailed semantics of this.
  scoped_ptr<SchedulerInterface> scheduler_;
  // The local object store.
  scoped_ptr<ObjectStoreInterface> object_store_;
#ifdef __SIMULATE_SYNTHETIC_DTG__
  shared_ptr<sim::SimpleDTGGenerator> sim_dtg_generator_;
#endif
};

}  // namespace firmament

#endif  // FIRMAMENT_ENGINE_COORDINATOR_H
