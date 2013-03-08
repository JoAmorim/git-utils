#include "repository.h"
#include <string>

using namespace v8;
using namespace std;

void Repository::Init(Handle<Object> target) {
  git_threads_init();

  Local<FunctionTemplate> tpl = FunctionTemplate::New(Repository::New);
  tpl->SetClassName(v8::String::NewSymbol("Repository"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("getPath"), FunctionTemplate::New(Repository::GetPath)->GetFunction());
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("exists"), FunctionTemplate::New(Repository::Exists)->GetFunction());
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("getHead"), FunctionTemplate::New(Repository::GetHead)->GetFunction());
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("refreshIndex"), FunctionTemplate::New(Repository::RefreshIndex)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(v8::String::NewSymbol("Repository"), constructor);
}

NODE_MODULE(git, Repository::Init)

Handle<Value> Repository::New(const Arguments& args) {
  HandleScope scope;
  Repository* repository = new Repository(Local<String>::Cast(args[0]));
  repository->Wrap(args.This());
  return args.This();
}

Handle<Value> Repository::Exists(const Arguments& args) {
  HandleScope scope;
  Repository* repository = node::ObjectWrap::Unwrap<Repository>(args.This());
  return scope.Close(Boolean::New(repository->repository != NULL));
}

Handle<Value> Repository::GetPath(const Arguments& args) {
  HandleScope scope;
  Repository* repository = node::ObjectWrap::Unwrap<Repository>(args.This());
  const char* path = repository->GetPath();
  return scope.Close(String::NewSymbol(path));
}

Handle<Value> Repository::GetHead(const Arguments& args) {
  HandleScope scope;
  git_repository* repository = node::ObjectWrap::Unwrap<Repository>(args.This())->repository;
  git_reference *head;
  if (git_repository_head(&head, repository) != GIT_OK)
    return scope.Close(Null());

  if (git_repository_head_detached(repository) == 1) {
    const git_oid* sha = git_reference_target(head);
    if (sha != NULL) {
      char oid[GIT_OID_HEXSZ + 1];
      git_oid_tostr(oid, GIT_OID_HEXSZ + 1, sha);
      git_reference_free(head);
      return scope.Close(String::NewSymbol(oid));
    }
  }

  Local<String> referenceName =  String::NewSymbol(git_reference_name(head));
  git_reference_free(head);
  return scope.Close(referenceName);
}

Handle<Value> Repository::RefreshIndex(const Arguments& args) {
  git_repository* repository = node::ObjectWrap::Unwrap<Repository>(args.This())->repository;
  git_index* index;
  if (git_repository_index(&index, repository) == GIT_OK) {
    git_index_read(index);
    git_index_free(index);
  }
  HandleScope scope;
  return scope.Close(Undefined());
}

Repository::Repository(Handle<String> path) {
  String::Utf8Value utf8Path(path);
  string repositoryPath(*utf8Path);
  if (git_repository_open_ext(&repository, repositoryPath.c_str(), 0, NULL) != GIT_OK)
    repository = NULL;
}

Repository::~Repository() {
  if (repository != NULL)
    git_repository_free(repository);
}

const char* Repository::GetPath() {
  return git_repository_path(repository);
}
