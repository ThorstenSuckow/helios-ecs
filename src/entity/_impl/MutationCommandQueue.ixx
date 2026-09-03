module;


#include <vector>
#include <memory>

export module helios.ecs.entity.EntityMutationManager:MutationCommandQueue;

import helios.ecs.entity.storage.SparseSet;
import helios.ecs.command.commands;
import helios.ecs.component.components;
import helios.ecs.entity.EntityManager;

export namespace helios::ecs::entity {

    template<typename THandle>
    class MutationCommandQueue {

        template<typename TComponent>
        using SparseSet = ecs::entity::storage::SparseSet<TComponent>;

        using EntityManager = ecs::entity::EntityManager<THandle>;

        template<typename TComponent>
        using AddComponentCommand = ecs::commands::AddComponentCommand<TComponent>;
        template<typename TComponent>
        using RemoveComponentCommand = ecs::commands::RemoveComponentCommand<TComponent>;

        class Concept {
        public:
            virtual ~Concept() = default;
            virtual void executeCommands(const EntityManager& entityManager) noexcept =0;
            virtual void addAddCommand(void* addCommand) noexcept = 0;
            virtual void addRemoveCommand(void* removeCommand) noexcept = 0;
            virtual void addAddCommands(void* addCommands) noexcept = 0;
            virtual void addRemoveCommands(void* removeCommands) noexcept = 0;
        };

        template<typename TComponent>
        class Model final : public Concept {
            std::vector<AddComponentCommand<TComponent>> addCommands_;
            std::vector<RemoveComponentCommand<TComponent>> removeCommands_;

            SparseSet<TComponent>* sparseSet_{nullptr};

        public:
            explicit Model(SparseSet<TComponent>* sparseSet) : sparseSet_(sparseSet) {}

            void addAddCommand(void* addCommand) noexcept override {
                auto& cmd = *static_cast<AddComponentCommand<TComponent>*>(addCommand);
                addCommands_.push_back(std::move(cmd));
            }

            void addRemoveCommand(void* removeCommand) noexcept override {
                auto& cmd = *static_cast<RemoveComponentCommand<TComponent>*>(removeCommand);
                removeCommands_.push_back(std::move(cmd));
            }

            void addAddCommands(void* addCommands) noexcept override {
                auto& cmd = *static_cast<std::vector<AddComponentCommand<TComponent>>*>(addCommands);
                if (addCommands_.empty()) {
                    addCommands_ = std::move(cmd);
                } else {
                    addCommands_.insert(addCommands_.end(),
                        std::make_move_iterator(cmd.begin()),
                        std::make_move_iterator(cmd.end()));
                }
            }

            void addRemoveCommands(void* removeCommands) noexcept override {
                auto& cmd = *static_cast<std::vector<RemoveComponentCommand<TComponent>>*>(removeCommands);
                if (removeCommands_.empty()) {
                    removeCommands_ = std::move(cmd);
                } else {
                    removeCommands_.insert(removeCommands_.end(),
                        std::make_move_iterator(cmd.begin()),
                        std::make_move_iterator(cmd.end()));
                }
            }

            void executeCommands(const EntityManager& entityManager) noexcept override {
                for (auto& cmd : addCommands_) {
                    if (!entityManager.isValid(cmd.handle)) {
                        continue;
                    }
                    sparseSet_->insert(cmd.handle.entityId(), std::move(cmd.component));
                }
                addCommands_.clear();

                for (auto& cmd : removeCommands_) {
                    if (!entityManager.isValid(cmd.handle)) {
                        continue;
                    }
                    sparseSet_->remove(cmd.handle.entityId());
                }
                removeCommands_.clear();

                sparseSet_->finalizeMutations();
            }
        };

        std::unique_ptr<Concept> model_;

        explicit MutationCommandQueue(std::unique_ptr<Concept> model) : model_(std::move(model)) {}
    public:

        template<typename TComponent>
        static MutationCommandQueue make(SparseSet<TComponent>* sparseSet) {
            return MutationCommandQueue{std::make_unique<Model<TComponent>>(sparseSet)};
        }

        template<typename TComponent>
        void add(AddComponentCommand<TComponent>&& command) {
            model_->addAddCommand(static_cast<void*>(std::addressof(command)));
        }

        template<typename TComponent>
        void add(RemoveComponentCommand<TComponent>&& command) {
            model_->addRemoveCommand(static_cast<void*>(std::addressof(command)));
        }

        // vectors
        template<typename TComponent>
        void add(std::vector<AddComponentCommand<TComponent>>&& commands) {
            model_->addAddCommands(static_cast<void*>(std::addressof(commands)));
        }

        template<typename TComponent>
        void add(std::vector<RemoveComponentCommand<TComponent>>&& commands) {
            model_->addRemoveCommands(static_cast<void*>(std::addressof(commands)));
        }

        void executeCommands(const EntityManager& entityManager) noexcept {
            model_->executeCommands(entityManager);
        }
    };
}