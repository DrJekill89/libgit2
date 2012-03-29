#include "clar_libgit2.h"

static const char* tagger_name = "Vicent Marti";
static const char* tagger_email = "vicent@github.com";
static const char* tagger_message = "This is my tag.\n\nThere are many tags, but this one is mine\n";

static const char *tag2_id = "7b4384978d2493e851f9cca7858815fac9b10980";
static const char *tagged_commit = "e90810b8df3e80c413d903f631643c716887138d";
static const char *bad_tag_id = "eda9f45a2a98d4c17a09d681d88569fa4ea91755";
static const char *badly_tagged_commit = "e90810b8df3e80c413d903f631643c716887138d";

static git_repository *g_repo;


// Fixture setup and teardown
void test_tag_write__initialize(void)
{
   g_repo = cl_git_sandbox_init("testrepo");
}

void test_tag_write__cleanup(void)
{
   cl_git_sandbox_cleanup();
}



void test_tag_write__basic(void)
{
   // write a tag to the repository and read it again
	git_tag *tag;
	git_oid target_id, tag_id;
	git_signature *tagger;
	const git_signature *tagger1;
	git_reference *ref_tag;
	git_object *target;

	git_oid_fromstr(&target_id, tagged_commit);
	cl_git_pass(git_object_lookup(&target, g_repo, &target_id, GIT_OBJ_COMMIT));

	/* create signature */
	cl_git_pass(git_signature_new(&tagger, tagger_name, tagger_email, 123456789, 60));

	cl_git_pass(git_tag_create(
                              &tag_id, /* out id */
                              g_repo,
                              "the-tag",
                              target,
                              tagger,
                              tagger_message,
                              0));

	git_object_free(target);
	git_signature_free(tagger);

	cl_git_pass(git_tag_lookup(&tag, g_repo, &tag_id));
	cl_assert(git_oid_cmp(git_tag_target_oid(tag), &target_id) == 0);

	/* Check attributes were set correctly */
	tagger1 = git_tag_tagger(tag);
	cl_assert(tagger1 != NULL);
	cl_assert(strcmp(tagger1->name, tagger_name) == 0);
	cl_assert(strcmp(tagger1->email, tagger_email) == 0);
	cl_assert(tagger1->when.time == 123456789);
	cl_assert(tagger1->when.offset == 60);

	cl_assert(strcmp(git_tag_message(tag), tagger_message) == 0);

	cl_git_pass(git_reference_lookup(&ref_tag, g_repo, "refs/tags/the-tag"));
	cl_assert(git_oid_cmp(git_reference_oid(ref_tag), &tag_id) == 0);
	cl_git_pass(git_reference_delete(ref_tag));
#ifndef GIT_WIN32
	cl_assert((loose_object_mode(REPOSITORY_FOLDER, (git_object *)tag) & 0777) == GIT_OBJECT_FILE_MODE);
#endif

	git_tag_free(tag);
}

void test_tag_write__overwrite(void)
{
   // Attempt to write a tag bearing the same name than an already existing tag
	git_oid target_id, tag_id;
	git_signature *tagger;
	git_object *target;

	git_oid_fromstr(&target_id, tagged_commit);
	cl_git_pass(git_object_lookup(&target, g_repo, &target_id, GIT_OBJ_COMMIT));

	/* create signature */
	cl_git_pass(git_signature_new(&tagger, tagger_name, tagger_email, 123456789, 60));

	cl_git_fail(git_tag_create(
                              &tag_id, /* out id */
                              g_repo,
                              "e90810b",
                              target,
                              tagger,
                              tagger_message,
                              0));

	git_object_free(target);
	git_signature_free(tagger);

}

void test_tag_write__replace(void)
{
   // Replace an already existing tag
	git_oid target_id, tag_id, old_tag_id;
	git_signature *tagger;
	git_reference *ref_tag;
	git_object *target;

	git_oid_fromstr(&target_id, tagged_commit);
	cl_git_pass(git_object_lookup(&target, g_repo, &target_id, GIT_OBJ_COMMIT));

	cl_git_pass(git_reference_lookup(&ref_tag, g_repo, "refs/tags/e90810b"));
	git_oid_cpy(&old_tag_id, git_reference_oid(ref_tag));
	git_reference_free(ref_tag);

	/* create signature */
	cl_git_pass(git_signature_new(&tagger, tagger_name, tagger_email, 123456789, 60));

	cl_git_pass(git_tag_create(
                              &tag_id, /* out id */
                              g_repo,
                              "e90810b",
                              target,
                              tagger,
                              tagger_message,
                              1));

	git_object_free(target);
	git_signature_free(tagger);

	cl_git_pass(git_reference_lookup(&ref_tag, g_repo, "refs/tags/e90810b"));
	cl_assert(git_oid_cmp(git_reference_oid(ref_tag), &tag_id) == 0);
	cl_assert(git_oid_cmp(git_reference_oid(ref_tag), &old_tag_id) != 0);

	git_reference_free(ref_tag);
}

void test_tag_write__lightweight(void)
{
   // write a lightweight tag to the repository and read it again
	git_oid target_id, object_id;
	git_reference *ref_tag;
	git_object *target;

	git_oid_fromstr(&target_id, tagged_commit);
	cl_git_pass(git_object_lookup(&target, g_repo, &target_id, GIT_OBJ_COMMIT));

	cl_git_pass(git_tag_create_lightweight(
                                          &object_id,
                                          g_repo,
                                          "light-tag",
                                          target,
                                          0));

	git_object_free(target);

	cl_assert(git_oid_cmp(&object_id, &target_id) == 0);

	cl_git_pass(git_reference_lookup(&ref_tag, g_repo, "refs/tags/light-tag"));
	cl_assert(git_oid_cmp(git_reference_oid(ref_tag), &target_id) == 0);

	cl_git_pass(git_tag_delete(g_repo, "light-tag"));

	git_reference_free(ref_tag);
}

void test_tag_write__lightweight_over_existing(void)
{
   // Attempt to write a lightweight tag bearing the same name than an already existing tag
	git_oid target_id, object_id, existing_object_id;
	git_object *target;

	git_oid_fromstr(&target_id, tagged_commit);
	cl_git_pass(git_object_lookup(&target, g_repo, &target_id, GIT_OBJ_COMMIT));

	cl_git_fail(git_tag_create_lightweight(
                                          &object_id,
                                          g_repo,
                                          "e90810b",
                                          target,
                                          0));

	git_oid_fromstr(&existing_object_id, tag2_id);
	cl_assert(git_oid_cmp(&object_id, &existing_object_id) == 0);

	git_object_free(target);
}

void test_tag_write__delete(void)
{
   // Delete an already existing tag
	git_reference *ref_tag;

	cl_git_pass(git_tag_delete(g_repo, "e90810b"));

	cl_git_fail(git_reference_lookup(&ref_tag, g_repo, "refs/tags/e90810b"));

	git_reference_free(ref_tag);
}
